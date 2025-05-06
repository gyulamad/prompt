#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "clang-c/Index.h"
#include "../utils/Arguments.hpp"
#include "../str/str_contains.h"
#include "../utils/ERROR.h"

using namespace std;
using namespace tools::utils;
using namespace tools::str;

namespace tools::utils {

class CodeSplitter {
private:
    string cxxStandard;    // e.g., "c++17"
    string headerExt;      // e.g., ".h" or ".hpp"
    string implExt;        // e.g., ".cpp"

    struct Function {
        string declaration;    // e.g., "void foo(int x)"
        string body;           // Function body (if definition)
        string namespaceName;  // e.g., "ns1::ns2"
        bool isDefinition;     // True if it has a body
    };

    struct FileOutput {
        vector<string> headerIncludes;
        vector<string> cppIncludes;
        vector<Function> functions;
        vector<string> declarations; // Non-function declarations (e.g., classes)
        string namespaceName;       // Main namespace
    };

    struct ParseContext {
        CodeSplitter* splitter;
        CXTranslationUnit tu;
        FileOutput* output;
    };

    static string toString(CXString cxStr) {
        string result = clang_getCString(cxStr);
        clang_disposeString(cxStr);
        return result;
    }

    string getSourceRange(CXSourceRange range) {
        CXSourceLocation start = clang_getRangeStart(range);
        CXSourceLocation end = clang_getRangeEnd(range);
        CXFile file;
        unsigned startOffset, endOffset;
        clang_getFileLocation(start, &file, nullptr, nullptr, &startOffset);
        clang_getFileLocation(end, &file, nullptr, nullptr, &endOffset);
        if (!file) throw ERROR("Cannot get file location");

        ifstream inFile(toString(clang_getFileName(file)));
        if (!inFile.is_open()) throw ERROR("Cannot open source file");
        inFile.seekg(startOffset);
        string content(endOffset - startOffset, '\0');
        inFile.read(&content[0], endOffset - startOffset);
        return content;
    }

    string getNamespace(CXCursor cursor) {
        string ns;
        while (cursor.kind != CXCursor_TranslationUnit) {
            if (cursor.kind == CXCursor_Namespace) {
                string name = toString(clang_getCursorSpelling(cursor));
                if (!ns.empty()) ns = name + "::" + ns;
                else ns = name;
            }
            cursor = clang_getCursorSemanticParent(cursor);
        }
        return ns;
    }

    static CXChildVisitResult visitAst(CXCursor cursor, CXCursor parent, CXClientData clientData) {
        (void)parent; // Suppress unused parameter warning
        ParseContext* context = static_cast<ParseContext*>(clientData);
        CodeSplitter* splitter = context->splitter;
        CXTranslationUnit tu = context->tu;
        FileOutput* output = context->output;
        CXSourceRange range = clang_getCursorExtent(cursor);

        if (cursor.kind == CXCursor_InclusionDirective) {
            string include = CodeSplitter::toString(clang_getCursorSpelling(cursor));
            if (include.find('<') != string::npos)
                output->headerIncludes.push_back("#include " + include);
            else
                output->headerIncludes.push_back("#include \"" + include + "\"");
        }
        else if (cursor.kind == CXCursor_Namespace) {
            string ns = splitter->getNamespace(cursor);
            if (!ns.empty() && output->namespaceName.empty()) output->namespaceName = ns;
        }
        else if (cursor.kind == CXCursor_FunctionDecl) {
            string decl = CodeSplitter::toString(clang_getCursorDisplayName(cursor));
            string ns = splitter->getNamespace(cursor);
            CXCursor definition = clang_getCursorDefinition(cursor);
            bool isDefinition = !clang_Cursor_isNull(definition) && clang_equalCursors(cursor, definition);

            Function func;
            func.declaration = decl;
            func.namespaceName = ns;
            func.isDefinition = isDefinition;
            if (isDefinition) {
                func.body = splitter->getSourceRange(range);
                size_t bracePos = func.body.find('{');
                if (bracePos != string::npos) func.body = func.body.substr(bracePos);
            }
            output->functions.push_back(func);
        }
        else if (cursor.kind == CXCursor_StructDecl || cursor.kind == CXCursor_ClassDecl) {
            string decl = splitter->getSourceRange(range);
            output->declarations.push_back(decl);
        }

        ParseContext childContext = {splitter, tu, output};
        clang_visitChildren(cursor, visitAst, &childContext);
        return CXChildVisit_Continue;
    }

public:
    CodeSplitter(string cxxStd = "c++17", string hExt = ".h", string iExt = ".cpp")
        : cxxStandard(cxxStd), headerExt(hExt), implExt(iExt) {
        if (!headerExt.empty() && headerExt[0] != '.') headerExt = "." + headerExt;
        if (!implExt.empty() && implExt[0] != '.') implExt = "." + implExt;
    }

    bool split(string inputFile, string outputDir) {
        CXIndex index = clang_createIndex(0, 0);
        if (!index) throw ERROR("Cannot create Clang index");
        CXTranslationUnit tu = nullptr;
        string stdFlag = "-std=" + cxxStandard;
        const char* args[] = {stdFlag.c_str()};
        int numArgs = sizeof(args) / sizeof(args[0]);
        if (clang_parseTranslationUnit2(index, inputFile.c_str(), args, numArgs, nullptr, 0,
                                       CXTranslationUnit_None, &tu))
            throw ERROR("Cannot parse " + inputFile);
        if (!tu) throw ERROR("Translation unit is null");

        FileOutput output;
        CXCursor root = clang_getTranslationUnitCursor(tu);
        ParseContext context = {this, tu, &output};
        clang_visitChildren(root, visitAst, &context);

        string outDir = outputDir.empty() ? filesystem::path(inputFile).parent_path().string() : outputDir;
        if (outDir.empty()) outDir = ".";
        filesystem::create_directories(outDir);

        string baseName = filesystem::path(inputFile).stem().string();
        string headerFile = outDir + "/" + baseName + headerExt;
        string cppFile = outDir + "/" + baseName + implExt;

        string inputAbsolute = filesystem::absolute(inputFile).string();
        if (filesystem::absolute(headerFile).string() == inputAbsolute ||
            filesystem::absolute(cppFile).string() == inputAbsolute)
            throw ERROR("Output file would overwrite input file: " + inputFile);

        ofstream hOut(headerFile);
        if (!hOut.is_open()) throw ERROR("Cannot open output file " + headerFile);
        hOut << "#pragma once\n\n";
        set<string> includes(output.headerIncludes.begin(), output.headerIncludes.end());
        for (string inc : includes) hOut << inc << "\n";
        hOut << "\n";
        set<string> namespaces;
        for (Function func : output.functions)
            if (!func.namespaceName.empty()) namespaces.insert(func.namespaceName);
        for (string ns : namespaces) {
            hOut << "namespace " << ns << " {\n\n";
            for (Function func : output.functions)
                if (func.namespaceName == ns) hOut << "    " << func.declaration << ";\n";
            hOut << "\n} // namespace " + ns + "\n\n";
        }
        for (string decl : output.declarations) hOut << decl << "\n\n";
        hOut.close();

        ofstream cppOut(cppFile);
        if (!cppOut.is_open()) throw ERROR("Cannot open output file " + cppFile);
        cppOut << "#include \"" << baseName << headerExt << "\"\n";
        cppOut << "#include <string>\n\n";
        cppOut << "using namespace std;\n\n";
        for (string ns : namespaces) {
            cppOut << "namespace " << ns << " {\n\n";
            for (Function func : output.functions)
                if (func.namespaceName == ns && func.isDefinition)
                    cppOut << func.declaration << " " << func.body << "\n\n";
            cppOut << "} // namespace " + ns + "\n\n";
        }
        cppOut.close();

        cout << "Generated " << headerFile << " and " << cppFile << endl;

        clang_disposeTranslationUnit(tu);
        tu = nullptr;
        clang_disposeIndex(index);
        return true;
    }

    bool split(vector<string> inputFiles, string outputDir) {
        bool success = true;
        for (string inputFile : inputFiles) {
            if (!filesystem::exists(inputFile)) {
                cerr << "Error: File does not exist: " << inputFile << endl;
                success = false;
                continue;
            }
            bool result = false;
            try {
                result = split(inputFile, outputDir);
            } catch (exception& e) {
                cerr << "Error processing " + inputFile + ": " + e.what() << endl;
                success = false;
            }
            if (!result) success = false;
        }
        return success;
    }
};

} // namespace tools::utils

#ifdef TEST

#include "../utils/Test.h"

using namespace tools::utils;

void test_CodeSplitter_split_single_file() {
    string inputFile = "test_input.hpp";
    string outputDir = "test_output";
    CodeSplitter splitter("c++17", ".hpp", ".cpp");

    ofstream input(inputFile);
    input << "#pragma once\n\n#include <string>\n\nnamespace test::ns {\n    void foo(int x);\n    std::string bar(const std::string& s) {\n        return s + \"!\";\n    }\n}\n";
    input.close();

    bool actual = splitter.split(inputFile, outputDir);
    bool existsHpp = filesystem::exists(outputDir + "/test_input.hpp");
    bool existsCpp = filesystem::exists(outputDir + "/test_input.cpp");
    bool expected = true;
    assert(actual == expected && existsHpp && existsCpp && "Failed to split single file");

    filesystem::remove(inputFile);
    filesystem::remove_all(outputDir);
}

void test_CodeSplitter_split_overwrite_protection() {
    string inputFile = "test_input.hpp";
    string outputDir = ".";
    CodeSplitter splitter("c++17", ".hpp", ".cpp");

    ofstream input(inputFile);
    input << "#pragma once\n\n#include <string>\n\nnamespace test::ns {\n    void foo(int x);\n}\n";
    input.close();

    bool thrown = false;
    string what;
    try {
        splitter.split(inputFile, outputDir);
    } catch (exception& e) {
        thrown = true;
        what = e.what();
    }
    bool expected = true;
    assert(thrown == expected && str_contains(what, "would overwrite input file") && "Overwrite protection failed");

    filesystem::remove(inputFile);
}

void test_CodeSplitter_split_nonexistent_file() {
    string inputFile = "nonexistent.hpp";
    string outputDir = "test_output";
    CodeSplitter splitter("c++17", ".hpp", ".cpp");

    bool actual = splitter.split(vector<string>{inputFile}, outputDir);
    bool expected = false;
    assert(actual == expected && "Should fail on nonexistent file");
}

TEST(test_CodeSplitter_split_single_file);
TEST(test_CodeSplitter_split_overwrite_protection);
TEST(test_CodeSplitter_split_nonexistent_file);

#endif

int main(int argc, char* argv[]) {
    Arguments args(argc, argv);

    if (!args.has(1)) {
        cerr << "Usage: code_splitter <input_files> [--output-dir <dir>] [--std <standard>] [--header-ext <ext>] [--impl-ext <ext>]\n";
        cerr << "  input_files: Comma-separated list of header files (required)\n";
        cerr << "  --output-dir: Optional output directory (defaults to input file's directory)\n";
        cerr << "  --std: Optional C++ standard (defaults to c++17)\n";
        cerr << "  --header-ext: Optional header extension (defaults to .h)\n";
        cerr << "  --impl-ext: Optional implementation extension (defaults to .cpp)\n";
        return 1;
    }

    string inputFiles = args.get<string>(1);
    vector<string> files;
    stringstream ss(inputFiles);
    string file;
    while (getline(ss, file, ','))
        if (!file.empty()) files.push_back(file);

    string defaultOutputDir = files.empty() ? "." : filesystem::path(files[0]).parent_path().string();
    string outputDir = args.has("output-dir") ? args.get<string>("output-dir") : defaultOutputDir;
    string cxxStandard = args.has("std") ? args.get<string>("std") : "c++17";
    string headerExt = args.has("header-ext") ? args.get<string>("header-ext") : ".h";
    string implExt = args.has("impl-ext") ? args.get<string>("impl-ext") : ".cpp";

    CodeSplitter splitter(cxxStandard, headerExt, implExt);
    if (!splitter.split(files, outputDir)) return 1;
    return 0;
}