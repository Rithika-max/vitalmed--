import express from 'express';
import multer from 'multer';
import sqlite3 from 'sqlite3';
import cors from 'cors';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { createRequire } from 'module';

const require = createRequire(import.meta.url);
const mammoth = require('mammoth');

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const app = express();

// Resolve storage root to absolute path for persistence across restarts
const storageRoot = path.resolve(__dirname, '../storage');
const uploadDir = path.join(storageRoot, 'uploads');
const extractedDir = path.join(storageRoot, 'extracted');
const logsDir = path.join(storageRoot, 'logs');

// Create directories if they don't exist
if (!fs.existsSync(storageRoot)) fs.mkdirSync(storageRoot, { recursive: true });
if (!fs.existsSync(uploadDir)) fs.mkdirSync(uploadDir, { recursive: true });
if (!fs.existsSync(extractedDir)) fs.mkdirSync(extractedDir, { recursive: true });
if (!fs.existsSync(logsDir)) fs.mkdirSync(logsDir, { recursive: true });

console.log(`📁 Storage root: ${storageRoot}`);
console.log(`📁 Uploads dir:  ${uploadDir}`);
console.log(`📁 Extracted dir: ${extractedDir}`);

const upload = multer({ dest: uploadDir });

app.use(cors());
app.use(express.json());

// ─── CSV Parser ───────────────────────────────────────────────────────────────
function parseCSV(content) {
  const lines = content.trim().split(/\r?\n/);
  if (lines.length < 2) return [];
  const headers = lines[0].split(',').map(h => h.trim().replace(/^"|"$/g, ''));
  const rows = [];
  for (let i = 1; i < lines.length; i++) {
    const line = lines[i].trim();
    if (!line) continue;
    const values = [];
    let current = '';
    let inQuotes = false;
    for (let j = 0; j < line.length; j++) {
      const char = line[j];
      if (char === '"') { inQuotes = !inQuotes; }
      else if (char === ',' && !inQuotes) { values.push(current.trim()); current = ''; }
      else { current += char; }
    }
    values.push(current.trim());
    const cleanValues = values.map(v => v.replace(/^"|"$/g, '').trim());
    if (cleanValues.length === headers.length) {
      const row = {};
      headers.forEach((h, idx) => { row[h] = cleanValues[idx]; });
      rows.push(row);
    }
  }
  return rows;
}

// ─── File Content Extractor ───────────────────────────────────────────────────
async function readFileContent(filePath, fileType) {
  try {
    if (fileType === 'pdf') {
      // pdf-parse v2 API: use PDFParse class with data buffer
      const { PDFParse } = await import('pdf-parse');
      const dataBuffer = fs.readFileSync(filePath);
      const parser = new PDFParse({ data: dataBuffer });
      const pdfData = await parser.getText();
      const text = (pdfData.text || '').trim();
      return text || '(No readable text found in this PDF)';
    }

    if (fileType === 'docx') {
      const result = await mammoth.extractRawText({ path: filePath });
      const text = (result.value || '').trim();
      return text || '(No readable text found in this Word document)';
    }

    // Text-based file types
    const content = fs.readFileSync(filePath, 'utf8');

    if (fileType === 'csv') {
      const rows = parseCSV(content);
      return rows.length > 0 ? rows : content; // fallback to raw text if parsing yields nothing
    }
    if (fileType === 'json') {
      return JSON.parse(content);
    }
    if (fileType === 'txt') {
      return content;
    }

    // Binary types we can't extract (excel etc.) — return a helpful note
    return `This is a binary ${fileType} file. Please convert to CSV, TXT, PDF, or DOCX for AI analysis.`;

  } catch (err) {
    console.error(`[readFileContent] Error for type="${fileType}":`, err.message);
    return null;
  }
}

// ─── Database ─────────────────────────────────────────────────────────────────
const dbPath = path.join(storageRoot, 'metadata.db');
console.log(`📊 Database path: ${dbPath}\n`);
const db = new sqlite3.Database(dbPath);

db.serialize(() => {
  db.run(`
    CREATE TABLE IF NOT EXISTS files_metadata (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      filename TEXT,
      file_type TEXT,
      file_path TEXT,
      extracted_path TEXT,
      uploaded_at TEXT,
      processed_status TEXT,
      schema TEXT,
      dataset_type TEXT,
      source TEXT,
      size_bytes INTEGER,
      extracted_size INTEGER
    )
  `);
});

// ─── Routes ───────────────────────────────────────────────────────────────────

// Health check
app.get('/health', (req, res) => {
  res.json({ status: 'ok', service: 'vetalmed-erp-ai-backend', version: '1.0.0' });
});

// Upload & process
app.post('/upload', upload.single('file'), async (req, res) => {
  if (!req.file) {
    return res.status(400).json({ success: false, message: 'No file uploaded.' });
  }

  const filename = req.file.originalname;
  const ext = path.extname(filename).toLowerCase();
  const filePath = req.file.path;
  const fileSize = req.file.size;
  const now = new Date().toISOString();

  // Determine file type from extension
  let fileType = 'unknown';
  if (ext === '.csv')  fileType = 'csv';
  else if (ext === '.json') fileType = 'json';
  else if (ext === '.txt')  fileType = 'txt';
  else if (ext === '.pdf')  fileType = 'pdf';
  else if (ext === '.docx') fileType = 'docx';
  else if (['.xls', '.xlsx'].includes(ext)) fileType = 'excel';

  console.log(`[upload] File: ${filename}  |  Type: ${fileType}  |  Path: ${filePath}`);

  // Extract readable content
  const rawContent = await readFileContent(filePath, fileType);
  if (rawContent === null) {
    return res.status(400).json({ success: false, message: 'Failed to read file content.' });
  }

  // Build extracted text to save
  let extractedContent = '';
  let schema = '';

  if (fileType === 'csv' && Array.isArray(rawContent)) {
    const cols = Object.keys(rawContent[0] || {});
    schema = cols.join(', ');
    extractedContent = [
      `CSV Data`,
      `Columns (${cols.length}): ${schema}`,
      `Rows: ${rawContent.length}`,
      '',
      'Data:',
      JSON.stringify(rawContent, null, 2)
    ].join('\n');
  } else if (fileType === 'json') {
    schema = 'JSON document';
    extractedContent = typeof rawContent === 'string'
      ? rawContent
      : JSON.stringify(rawContent, null, 2);
  } else if (typeof rawContent === 'string') {
    // PDF, DOCX, TXT, fallback
    schema = `${fileType} document`;
    extractedContent = rawContent;
  } else {
    schema = `${fileType} document`;
    extractedContent = JSON.stringify(rawContent);
  }

  // Save extracted text to disk
  const extractedPath = path.join(extractedDir, filename + '.txt');
  fs.writeFileSync(extractedPath, extractedContent, 'utf8');
  const extractedSize = fs.statSync(extractedPath).size;

  const datasetType = ['csv', 'json', 'excel'].includes(fileType) ? 'structured' : 'unstructured';

  db.run(
    'INSERT INTO files_metadata(filename,file_type,file_path,extracted_path,uploaded_at,processed_status,schema,dataset_type,source,size_bytes,extracted_size) VALUES(?,?,?,?,?,?,?,?,?,?,?)',
    [filename, fileType, filePath, extractedPath, now, 'processed', schema, datasetType, 'upload', fileSize, extractedSize],
    function (err) {
      if (err) {
        console.error('[upload] DB error:', err);
        return res.status(500).json({ success: false, message: 'Database error.' });
      }
      console.log(`[upload] ✅ Saved: ${filename} (${fileType})`);
      res.json({
        success: true,
        message: 'File uploaded and processed successfully.',
        filename,
        file_type: fileType,
        status: 'processed',
        pipeline: datasetType,
        metadata_id: this.lastID
      });
    }
  );
});

// ─── Ollama Integration ───────────────────────────────────────────────────────
const OLLAMA_URL = 'http://localhost:11434/api/generate';
const OLLAMA_MODEL = 'tinyllama';

async function askOllama(prompt, context) {
  const body = JSON.stringify({
    model: OLLAMA_MODEL,
    prompt: `You are VetalMed ERP AI Assistant. Answer ONLY from the data below. Be brief and direct.\n\nDATA:\n${context}\n\nQuestion: ${prompt}\nAnswer:`,
    stream: false,
    options: { temperature: 0.2, num_predict: 200, num_ctx: 2048 }
  });

  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 60000); // 60s timeout

  try {
    const response = await fetch(OLLAMA_URL, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body,
      signal: controller.signal
    });

    if (!response.ok) {
      throw new Error(`Ollama returned ${response.status}: ${response.statusText}`);
    }

    const data = await response.json();
    return data.response || 'No response from AI model.';
  } finally {
    clearTimeout(timeout);
  }
}

// Smart context: extract only query-relevant lines from file content
function extractRelevantContext(content, query, maxChars = 1500) {
  const queryWords = query.toLowerCase().split(/\s+/).filter(w => w.length > 2);
  const lines = content.split('\n').filter(l => l.trim().length > 3);

  // Score each line by how many query words it contains
  const scored = lines.map(line => ({
    line,
    score: queryWords.filter(w => line.toLowerCase().includes(w)).length
  }));

  // Take relevant lines first, then fill with top lines for context
  const relevant = scored.filter(s => s.score > 0).map(s => s.line);
  const topLines = lines.slice(0, 10); // always include header/intro

  const combined = [...new Set([...topLines, ...relevant])];
  let result = '';
  for (const line of combined) {
    if (result.length + line.length > maxChars) break;
    result += line + '\n';
  }
  return result || content.substring(0, maxChars);
}

// Chat (Ollama-powered)
app.post('/chat', async (req, res) => {
  const { query } = req.body;
  if (!query || !query.trim()) {
    return res.json({ summary: 'Please ask a question.', data: [], insights: [], recommendation: '', confidence: 0.1, source_type: 'none' });
  }

  try {
    // Gather context from processed files (deduplicated by filename)
    const rows = await new Promise((resolve, reject) => {
      db.all(
        'SELECT filename, file_type, extracted_path FROM files_metadata WHERE processed_status = "processed" ORDER BY id DESC',
        (err, rows) => err ? reject(err) : resolve(rows || [])
      );
    });

    if (rows.length === 0) {
      return res.json({ summary: 'No data uploaded yet. Please upload a file first.', data: [], insights: [], recommendation: 'Upload a CSV, PDF, DOCX, or TXT file.', confidence: 0.1, source_type: 'no_data' });
    }

    // Deduplicate: keep only the latest version of each file
    const seen = new Set();
    const uniqueRows = rows.filter(r => {
      if (seen.has(r.filename)) return false;
      seen.add(r.filename);
      return true;
    });

    // Build compact context with only relevant content
    let context = '';
    const sources = [];
    const MAX_CONTEXT = 2000;

    for (const row of uniqueRows) {
      if (context.length >= MAX_CONTEXT) break;
      if (!fs.existsSync(row.extracted_path)) continue;

      const content = fs.readFileSync(row.extracted_path, 'utf8');
      if (!content) continue;

      const relevant = extractRelevantContext(content, query, MAX_CONTEXT - context.length);
      context += `[${row.filename}]\n${relevant}\n`;
      sources.push(row.filename);
    }

    if (!context.trim()) {
      return res.json({ summary: 'Uploaded files could not be read.', data: [], insights: [], recommendation: 'Try re-uploading your files.', confidence: 0.1, source_type: 'no_data' });
    }

    console.log(`[chat] Query: "${query}" | Sources: ${sources.join(', ')} | Context: ${context.length} chars`);

    const startTime = Date.now();
    const aiResponse = await askOllama(query, context);
    const elapsed = ((Date.now() - startTime) / 1000).toFixed(1);

    console.log(`[chat] ✅ Ollama responded in ${elapsed}s (${aiResponse.length} chars)`);

    res.json({
      summary: aiResponse,
      data: [],
      insights: sources.map(s => `Source: ${s}`),
      recommendation: 'Ask follow-up questions for deeper analysis.',
      confidence: 0.9,
      source_type: 'ai_generated'
    });

  } catch (err) {
    console.error('[chat] Ollama error:', err.message);

    // Fallback: if Ollama is unavailable, return a helpful message
    if (err.message.includes('ECONNREFUSED') || err.message.includes('fetch')) {
      return res.json({
        summary: 'AI model (Ollama) is not running. Please start Ollama with: ollama serve',
        data: [],
        insights: ['Ollama service not detected on localhost:11434'],
        recommendation: 'Run "ollama serve" in a terminal, then try again.',
        confidence: 0.1,
        source_type: 'error'
      });
    }

    res.json({
      summary: `AI error: ${err.message}`,
      data: [],
      insights: [],
      recommendation: 'Try again or rephrase your question.',
      confidence: 0.1,
      source_type: 'error'
    });
  }
});

// Datasets list
app.get('/datasets', (req, res) => {
  db.all('SELECT id, filename as file_name, file_type, file_path as original_path, extracted_path, extracted_size, processed_status, uploaded_at FROM files_metadata ORDER BY id DESC', (err, rows) => {
    if (err) return res.json([]);
    res.json(rows || []);
  });
});

// Delete file
app.delete('/datasets/:id', (req, res) => {
  const id = req.params.id;

  db.get('SELECT file_path, extracted_path, filename FROM files_metadata WHERE id = ?', [id], (err, row) => {
    if (err) {
      return res.status(500).json({ success: false, message: 'Database error.' });
    }

    if (!row) {
      return res.status(404).json({ success: false, message: 'File not found.' });
    }

    // Delete physical files
    try {
      if (row.file_path && fs.existsSync(row.file_path)) {
        fs.unlinkSync(row.file_path);
        console.log(`[delete] Deleted uploaded file: ${row.file_path}`);
      }

      if (row.extracted_path && fs.existsSync(row.extracted_path)) {
        fs.unlinkSync(row.extracted_path);
        console.log(`[delete] Deleted extracted file: ${row.extracted_path}`);
      }
    } catch (fsErr) {
      console.error('[delete] File deletion error:', fsErr.message);
      return res.status(500).json({ success: false, message: 'Failed to delete files from storage.' });
    }

    // Delete database record
    db.run('DELETE FROM files_metadata WHERE id = ?', [id], function (dbErr) {
      if (dbErr) {
        console.error('[delete] DB deletion error:', dbErr);
        return res.status(500).json({ success: false, message: 'Failed to delete file record.' });
      }

      console.log(`[delete] ✅ Deleted file: ${row.filename} (ID: ${id})`);
      res.json({ success: true, message: `File "${row.filename}" deleted successfully.` });
    });
  });
});

// ─── Start ────────────────────────────────────────────────────────────────────
const PORT = 8081;
app.listen(PORT, () => {
  console.log(`\n✅ VetalMed Backend running on http://localhost:${PORT}`);
  console.log(`   POST /upload  |  POST /chat  |  GET /datasets  |  DELETE /datasets/:id  |  GET /health\n`);
});
