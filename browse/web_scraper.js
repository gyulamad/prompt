#!/usr/bin/env node

const puppeteer = require('puppeteer-extra');
const StealthPlugin = require('puppeteer-extra-plugin-stealth');

puppeteer.use(StealthPlugin());

async function fetchPage({ url, method = 'GET', data = '', cookies = '', scriptCode = '' }) {
    if (!url.startsWith('http')) {
        console.error("Invalid URL. Ensure it starts with 'http://' or 'https://'.");
        process.exit(1);
    }

    const browser = await puppeteer.launch({ headless: true });
    const page = await browser.newPage();

    await page.setUserAgent('Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/110.0.0.0 Safari/537.36');

    // Set cookies if provided
    if (cookies) {
        const parsedCookies = cookies.split(';').map(cookie => {
            const [name, ...value] = cookie.split('=');
            return { name: name.trim(), value: value.join('=').trim(), domain: new URL(url).hostname };
        });
        await page.setCookie(...parsedCookies);
    }

    try {
        // Handle different HTTP methods
        if (method.toUpperCase() === 'GET') {
            await page.goto(url, { waitUntil: 'networkidle2', timeout: 60000 });
        } else {
            const postData = method.toUpperCase() === 'POST' || method.toUpperCase() === 'PUT' ? data : '';
            await page.setRequestInterception(true);
            page.on('request', req => {
                req.continue({
                    method,
                    postData,
                    headers: { ...req.headers(), 'Content-Type': 'application/x-www-form-urlencoded' }
                });
            });
            await page.goto(url, { waitUntil: 'networkidle2', timeout: 60000 });
        }

        // Inject JavaScript code if provided
        let scriptResult = null;
        if (scriptCode) {
            try {
                scriptResult = await page.evaluate(new Function(scriptCode));
                console.log("Script Result:", scriptResult);
            } catch (err) {
                console.error("Script Execution Error:", err.message);
            }
        }

        // Get fully rendered HTML
        // const content = await page.content();
        // console.log(content);

        // Extract human-readable text and links
        let extractedData;
        try {
            console.log("DEBUG: page.evaluate start...");
            extractedData = await page.evaluate(() => {
                console.log("DEBUG: Remove unnecessary elements...");
                // Remove unnecessary elements
                const removals = ['script', 'style', 'noscript', 'svg', 'iframe'];
                removals.forEach(selector => {
                    document.querySelectorAll(selector).forEach(el => el.remove());
                });

                console.log("DEBUG: Extract cleaned text content...");
                // Extract cleaned text content
                const getVisibleText = element => {
                    const style = window.getComputedStyle(element);
                    if (style.display === 'none' || style.visibility === 'hidden' || element.hidden)
                        return '';
                    return element.textContent;
                };

                const bodyText = getVisibleText(document.body);
                const cleanedText = bodyText
                    .replace(/\s+/g, ' ')
                    .replace(/[\r\n]+/g, '\n')
                    .trim();

                console.log("DEBUG: Extract and resolve links...");
                // Extract and resolve links
                const links = Array.from(document.querySelectorAll('a[href]'))
                    .map(a => {
                        try {
                            const text = a.textContent.replace(/\s+/g, ' ').trim();
                            const url = new URL(a.href, document.location.href).href;
                            return { text, target: url };
                        } catch (error) {
                            return null;
                        }
                    })
                    .filter(link => link && link.target.startsWith('http'));
                    
                console.log("DEBUG: return...");
                return {
                    //url: url,
                    readables: cleanedText,
                    links: [...new Map(links.map(item => [item.target, item])).values()] // Remove duplicates
                };
            });
        } catch (error) {
            console.error("Extraction failed:", error.message, error);
            extractedData = { /*url: url,*/ readables: "", links: [] };
        }

        console.log(JSON.stringify(extractedData, null, 2));


    } catch (error) {
        console.error("Failed to fetch the page:", error.message);
    } finally {
        await browser.close();
    }
}

// Parse CLI arguments
const args = process.argv.slice(2);
const options = {
    url: '',
    method: 'GET',
    data: '',
    cookies: '',
    scriptCode: ''
};

async function scrapeGoogle(query, maxPages = 1) {
    const browser = await puppeteer.launch({ headless: true });
    const page = await browser.newPage();

    await page.setUserAgent('Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/110.0.0.0 Safari/537.36');
    await page.goto('https://www.google.com', { waitUntil: 'load', timeout: 60000 });

    try {
        await page.waitForSelector('input[name="q"]');
        await page.type('input[name="q"]', query, { delay: 100 });
        await page.keyboard.press('Enter');
    } catch (e) {
        console.error("Google search failed.");
        await browser.close();
        return;
    }

    let results = [];
    let currentPage = 1;

    while (currentPage <= maxPages) {
        await page.waitForSelector('#search');

        let pageResults = await page.evaluate(() => {
            return [...document.querySelectorAll('.tF2Cxc')].map(el => ({
                title: el.querySelector('h3')?.innerText || 'No title',
                link: el.querySelector('a')?.href || 'No link',
                description: el.querySelector('.VwiC3b')?.innerText || 'No description'
            }));
        });

        results.push(...pageResults);

        const nextPageButton = await page.$('#pnnext');
        if (nextPageButton) {
            await Promise.all([
                nextPageButton.click(),
                page.waitForNavigation({ waitUntil: 'load', timeout: 10000 })
            ]);
            currentPage++;
        } else {
            break;
        }
    }

    fs.writeFileSync('google_results.json', JSON.stringify(results, null, 2), 'utf-8');
    console.log('Google results saved to google_results.json');
    await browser.close();
}

for (let i = 0; i < args.length; i++) {
    if (args[i] === '--url') options.url = args[i + 1];
    if (args[i] === '--method') options.method = args[i + 1];
    if (args[i] === '--data') options.data = args[i + 1];
    if (args[i] === '--cookies') options.cookies = args[i + 1];
    if (args[i] === '--script') options.scriptCode = args[i + 1];
    if (args[i] === '--google') options.googleQuery = args[i + 1];
}

if (options.googleQuery) {
    scrapeGoogle(options.googleQuery, options.googlePages);
} else if (options.url) {
    fetchPage(options);
} else {
    console.log("Usage:");
    console.log("  node web_scraper.js --url <url> [--method <METHOD>] [--data <DATA>] [--cookies <COOKIES>]");
    console.log("  node web_scraper.js --google <query> [--pages <number>]");
    process.exit(1);
}
