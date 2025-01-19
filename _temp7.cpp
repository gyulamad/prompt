#include <iostream>

using namespace std;

int main() {
    // Text colors
    cout << "\033[30mBlack text\033[0m" << endl;
    cout << "\033[31mRed text\033[0m" << endl;
    cout << "\033[32mGreen text\033[0m" << endl;
    cout << "\033[33mYellow text\033[0m" << endl;
    cout << "\033[34mBlue text\033[0m" << endl;
    cout << "\033[35mMagenta text\033[0m" << endl;
    cout << "\033[36mCyan text\033[0m" << endl;
    cout << "\033[37mWhite text\033[0m" << endl;

    // Bright text colors
    cout << "\033[90mBright Black\033[0m" << endl;
    cout << "\033[91mBright Red\033[0m" << endl;
    cout << "\033[92mBright Green\033[0m" << endl;
    cout << "\033[93mBright Yellow\033[0m" << endl;
    cout << "\033[94mBright Blue\033[0m" << endl;
    cout << "\033[95mBright Magenta\033[0m" << endl;
    cout << "\033[96mBright Cyan\033[0m" << endl;
    cout << "\033[97mBright White\033[0m" << endl;

    // Background colors
    cout << "\033[40mBlack background\033[0m" << endl;
    cout << "\033[41mRed background\033[0m" << endl;
    cout << "\033[42mGreen background\033[0m" << endl;
    cout << "\033[43mYellow background\033[0m" << endl;
    cout << "\033[44mBlue background\033[0m" << endl;
    cout << "\033[45mMagenta background\033[0m" << endl;
    cout << "\033[46mCyan background\033[0m" << endl;
    cout << "\033[47mWhite background\033[0m" << endl;

    // Bright background colors
    cout << "\033[100mBright Black background\033[0m" << endl;
    cout << "\033[101mBright Red background\033[0m" << endl;
    cout << "\033[102mBright Green background\033[0m" << endl;
    cout << "\033[103mBright Yellow background\033[0m" << endl;
    cout << "\033[104mBright Blue background\033[0m" << endl;
    cout << "\033[105mBright Magenta background\033[0m" << endl;
    cout << "\033[106mBright Cyan background\033[0m" << endl;
    cout << "\033[107mBright White background\033[0m" << endl;

    // Text attributes
    cout << "\033[1mBold text\033[0m" << endl;
    cout << "\033[2mDim text\033[0m" << endl;
    cout << "\033[3mItalic text\033[0m" << endl;
    cout << "\033[4mUnderlined text\033[0m" << endl;
    cout << "\033[5mBlinking text\033[0m" << endl;
    cout << "\033[7mInverted text\033[0m" << endl;
    cout << "\033[8mHidden text\033[0m" << endl;
    cout << "\033[9mStrikethrough text\033[0m" << endl;

    return 0;
}