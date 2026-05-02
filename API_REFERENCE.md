# VetalMed Backend API Reference

## Health Check

### GET /health

Check backend and Ollama status.

**Response (200 OK)**:
```json
{
  "backend": "running",
  "storage": "ok",
  "datasets_loaded": true,
  "dataset_count": 5,
  "ollama": "online|offline",
  "mode": "llm_enabled|rule_based",
  "service": "vetalmed-erp-ai-backend",
  "version": "1.0.0"
}
```

**Usage**: Frontend can check this to determine if LLM features are available.

---

## Datasets Management

### GET /datasets

Retrieve all uploaded and processed datasets.

**Response (200 OK)**:
```json
[
  {
    "id": 1,
    "file_name": "sales_data.csv",
    "file_type": "csv",
    "original_path": "storage/uploads/sales_data.csv",
    "extracted_path": "storage/extracted/sales_data.csv.txt",
    "extracted_size": 2048,
    "processed_status": "processed",
    "status": "processed",
    "uploaded_at": "2026-05-01 14:30:00 UTC"
  }
]
```

**Usage**: 
- Frontend calls this on page load
- Frontend calls this after upload/delete
- Restores dataset list after refresh

---

### DELETE /datasets/{id}

Delete a dataset by ID.

**Request**: DELETE /datasets/1

**Response (200 OK)**:
```json
{
  "success": true,
  "message": "Dataset deleted successfully"
}
```

**Response (500 Error)**:
```json
{
  "success": false,
  "message": "Error description"
}
```

---

## Upload

### POST /upload

Upload and process a new file.

**Request**:
- Method: POST
- Content-Type: multipart/form-data
- Body: file field with file data

**Response (200 OK)**:
```json
{
  "success": true,
  "message": "File uploaded and processed successfully.",
  "filename": "data.csv",
  "file_type": "csv",
  "status": "processed",
  "pipeline": "structured",
  "metadata_id": 42
}
```

**Response (400 Bad Request)**:
```json
{
  "success": false,
  "message": "Error description"
}
```

**Note**: Status is ALWAYS "processed" on success. Failures return error response, never a failed status.

---

## Chat

### POST /chat

Send a query to the AI assistant.

**Request**:
```json
{
  "query": "What is our top product?",
  "session_id": "default"
}
```

**Response (200 OK)**:
```json
{
  "summary": "Based on Q4 data, Product X was the top seller with 5,000 units.",
  "data": [
    {"product": "Product X", "units": 5000, "revenue": 500000}
  ],
  "insights": ["Product X dominates market", "Sales grew 15% YoY"],
  "recommendation": "Continue investing in Product X marketing",
  "confidence": 0.85,
  "source_type": "structured|rag|hybrid|rule_based|insufficient_data",
  "ollama_status": "online|offline"
}
```

**Behavior with Ollama Offline**:
- Chat still works
- Returns rule-based responses from uploaded data
- source_type changes to "rule_based" instead of "hybrid"
- confidence may be slightly lower
- No AI enhancement, but data is still returned

**Behavior with No Data**:
```json
{
  "summary": "No relevant data found for this question.",
  "data": [],
  "insights": [],
  "recommendation": "Please upload relevant data.",
  "confidence": 0.1,
  "source_type": "insufficient_data",
  "ollama_status": "online|offline"
}
```

---

## Frontend Integration Examples

### 1. Load datasets on page mount
```javascript
useEffect(() => {
  fetchDatasets();
}, []);

const fetchDatasets = async () => {
  const response = await axios.get(`${API_BASE}/datasets`);
  setDatasets(response.data);
};
```

### 2. Check if Ollama is available
```javascript
const checkHealth = async () => {
  const response = await axios.get(`${API_BASE}/health`);
  const ollamaOnline = response.data.ollama === "online";
  const mode = response.data.mode; // "llm_enabled" or "rule_based"
};
```

### 3. Upload a file
```javascript
const handleUpload = async () => {
  const formData = new FormData();
  formData.append('file', selectedFile);
  
  const response = await axios.post(`${API_BASE}/upload`, formData);
  
  if (response.data.success) {
    // File was processed
    // Reload datasets to show new file
    await fetchDatasets();
  }
};
```

### 4. Delete a dataset
```javascript
const handleDelete = async (fileId, fileName) => {
  await axios.delete(`${API_BASE}/datasets/${fileId}`);
  // Reload datasets
  await fetchDatasets();
};
```

### 5. Chat with fallback handling
```javascript
const sendChat = async (query) => {
  const response = await axios.post(`${API_BASE}/chat`, {
    query: query,
    session_id: 'default'
  });
  
  const data = response.data;
  
  // Check if data is from LLM or rule-based
  if (data.source_type === 'insufficient_data') {
    showMessage("No data available. Please upload relevant files.");
  } else if (data.ollama_status === 'offline' && data.source_type !== 'insufficient_data') {
    showMessage("AI is offline but returning data-based answer.");
  }
  
  displayResponse(data.summary);
};
```

---

## Logging

All backend operations are logged to: `storage/logs/backend.log`

Format: `[TIMESTAMP] [LEVEL] [COMPONENT] MESSAGE`

Example log entries:
```
[2026-05-01 14:30:00] [INFO] [Main] === Backend Starting ===
[2026-05-01 14:30:00] [INFO] [Main] Storage folders initialized
[2026-05-01 14:30:01] [INFO] [UploadController] File received: data.csv at storage/uploads/data.csv
[2026-05-01 14:30:01] [INFO] [UploadController] Detected file type: csv
[2026-05-01 14:30:01] [INFO] [UploadController] Content extracted, size: 2048 bytes
[2026-05-01 14:30:01] [INFO] [UploadController] File metadata saved with id: 42
[2026-05-01 14:30:05] [INFO] [HealthController] Ollama status checked: online
[2026-05-01 14:30:10] [INFO] [ChatController] Query received: What is our top product?
[2026-05-01 14:30:10] [INFO] [ChatController] Response generated successfully
```

---

## Expected Behavior After Fixes

### Upload Flow
1. User selects file
2. Frontend sends to POST /upload
3. Backend processes file (independent of Ollama)
4. Backend returns status "processed"
5. Frontend calls GET /datasets
6. File appears in dataset list
7. User refreshes page
8. Frontend calls GET /datasets again
9. File still appears (restored from SQLite)

### Chat Flow
1. User asks question
2. Frontend sends to POST /chat
3. Backend searches datasets
4. If data found:
   - If Ollama online: Enhance with AI
   - If Ollama offline: Return rule-based answer
5. Response always includes ollama_status
6. Frontend displays answer regardless of Ollama status

### Backend Restart
1. Backend starts
2. Creates storage folders (if missing)
3. Loads existing dataset metadata from SQLite
4. All previously uploaded files immediately available
5. No re-upload needed

---

## Status Codes

- `200` - Success
- `400` - Bad request (invalid input)
- `500` - Server error
- CORS headers added to all responses

---

## Notes

- All file paths in responses are relative to backend working directory
- Timestamps in responses are UTC
- Ollama status is checked dynamically, not cached
- Dataset list is always fresh from SQLite database
- Logs are rotated (consider implementing log rotation for production)
