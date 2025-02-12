# Web Scraper Setup and Usage Guide

## 1️⃣ Installation Steps (Linux)

### 🔹 Install Node.js (if not installed)

Check your Node.js version:

```bash
node -v
```

If it's below **16**, upgrade:

```bash
curl -fsSL https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.4/install.sh | bash
source ~/.bashrc  # or source ~/.zshrc
nvm install 18
nvm use 18
```

Verify installation:

```bash
node -v
```

### 🔹 Install Dependencies

1. **Go to your project folder**:

   ```bash
   cd /path/to/project
   ```

2. **Install Puppeteer (Full Version with Chromium)**

   ```bash
   npm install puppeteer
   ```

3. **(Alternative) Install Puppeteer-Core and Chrome manually**
   If you want to use an existing Chrome installation:

   ```bash
   npm install puppeteer-core
   npx puppeteer browsers install chrome
   ```

   Find Chrome’s path:

   ```bash
   which google-chrome
   ```

   Then, pass it as an argument when running the script.

---

## 2️⃣ Functionalities of the Web Scraper

1. **Visit a website and return its fully rendered HTML**
2. **Perform Google searches and extract results**
3. **Make HTTP requests with GET, POST, PUT, DELETE support**
4. **Inject JavaScript into pages and return execution results**
5. **Use cookies for session handling**

---

## 3️⃣ Usage Examples

### **🔹 Basic Usage: Fetch Rendered HTML of a Page**

```bash
node web_scraper.js --url "https://example.com"
```

### **🔹 Execute JavaScript on a Page**

```bash
node web_scraper.js --url "https://example.com" --script "return document.title;"
```

### **🔹 Perform a Google Search**

```bash
node web_scraper.js --google "Web scraping with Puppeteer" --pages 2
```

### **🔹 Make an HTTP POST Request with Cookies**

```bash
node web_scraper.js --url "https://example.com/api" --method "POST" --data '{"key": "value"}' --cookies "session_id=abc123"
```

### **🔹 Use Custom Chrome Path (If Using puppeteer-core)**

```bash
node web_scraper.js --url "https://example.com" --chrome-path "/usr/bin/google-chrome"
```

