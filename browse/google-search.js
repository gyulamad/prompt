#!/usr/bin/env node

const axios = require('axios');
const fs = require('fs');

// Load API key and CX from the JSON file
const secretsPath = './google-search-secret.json'; // Path to your JSON file
let API_KEY, CX;

try {
    const secrets = JSON.parse(fs.readFileSync(secretsPath, 'utf8'));
    API_KEY = secrets.secret; // Replace with the key in your JSON file
    CX = secrets.cx; // Replace with the key in your JSON file
} catch (error) {
    console.error("Failed to load API key and CX from the JSON file:", error.message);
    process.exit(1);
}


// Function to fetch Google search results using the API
async function fetchGoogleResults(query, maxResults = 10) {
    const url = `https://www.googleapis.com/customsearch/v1`;
    const params = {
        q: query,
        key: API_KEY,
        cx: CX,
        num: maxResults, // Number of results to fetch
    };

    try {
        const response = await axios.get(url, { params });
        const results = response.data.items.map(item => ({
            title: item.title,
            link: item.link,
            description: item.snippet,
        }));
        return results;
    } catch (error) {
        console.error("Failed to fetch Google results:", error.message);
        return [];
    }
}

// Function to save results to a JSON file
function saveResultsToFile(results, filename = 'google_results.json') {
    fs.writeFileSync(filename, JSON.stringify(results, null, 2), 'utf-8');
    console.log(`Results saved to ${filename}`);
}

// Parse CLI arguments
const args = process.argv.slice(2);
const options = {
    query: '',
    maxResults: 10,
};

for (let i = 0; i < args.length; i++) {
    if (args[i] === '--query') options.query = args[i + 1];
    if (args[i] === '--max') options.maxResults = parseInt(args[i + 1], 10);
}

// Main function
async function main() {
    if (!options.query) {
        console.log("Usage:");
        console.log("  node web_scraper.js --query <search_query> [--max <max_results>]");
        process.exit(1);
    }

    const results = await fetchGoogleResults(options.query, options.maxResults);
    if (results.length > 0) {
        console.log("Search Results:");
        results.forEach((result, index) => {
            console.log(`${index + 1}. ${result.title}`);
            console.log(`   ${result.link}`);
            console.log(`   ${result.description}`);
            console.log();
        });
        saveResultsToFile(results);
    } else {
        console.log("No results found.");
    }
}

main();