# Upload Failed - Troubleshooting Guide

## Quick Diagnosis

### Step 1: Is the Backend Running?

**On Windows (PowerShell)**:
```powershell
# Check if port 8080 is listening
netstat -ano | findstr :8080

# Or check if port 8081 is listening
netstat -ano | findstr :8081

# Test connection
Invoke-WebRequest -Uri "http://localhost:8080/health" -ErrorAction SilentlyContinue
```

**On Mac/Linux**:
```bash
netstat -tuln | grep 8080
curl http://localhost:8080/health
```

### Step 2: Check Frontend API Configuration

Frontend is configured to use port **8081**:
- File: `frontend/src/App.jsx`
- Line: `const API_BASE = 'http://localhost:8081';`

Check if backend should be on 8081 or if there's a proxy.

### Step 3: Build the Backend

If backend is not running, you need to build it first:

```bash
cd core
mkdir -p build
cd build

# On Windows with Visual Studio:
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release

# On Windows with LLVM/MinGW (if that's your compiler):
cmake .. -G "Unix Makefiles"
make

# Run:
./erp_ai_backend  (on Linux/Mac)
erp_ai_backend.exe  (on Windows)
```

### Step 4: Check Browser Console Errors

1. Open Chrome/Firefox DevTools (F12)
2. Go to "Console" tab
3. Try uploading a file
4. Look for error messages like:
   - `Failed to fetch` - Backend not running
   - `CORS error` - Frontend/backend port mismatch
   - `404 Not Found` - Wrong endpoint path

### Step 5: Check Network Tab

1. DevTools → Network tab
2. Upload a file
3. Look for the request to `/upload`
4. Check:
   - Status code (should be 200)
   - Response body (should have `"success": true`)

## Common Issues & Fixes

### Issue 1: "Failed to fetch" Error

**Cause**: Backend is not running or not accessible

**Fix**:
```bash
cd core/build
./erp_ai_backend
# You should see: "Drogon app running..." message
```

### Issue 2: "Cannot POST /upload" 404 Error

**Cause**: Endpoint not registered or wrong port

**Fix**:
- Verify backend is running
- Check if port matches frontend (8081 vs 8080)
- Check if UploadController is compiled

### Issue 3: 500 Internal Server Error

**Cause**: Backend error during processing

**Fix**:
- Check storage folders exist: `storage/uploads`, `storage/extracted`, `storage/logs`
- Check backend console output for error messages
- Check `storage/logs/backend.log` for details

### Issue 4: CORS Error (even with same-origin)

**Cause**: Missing CORS headers

**Fix**:
- Verify main.cpp has CORS headers:
```cpp
resp->addHeader("Access-Control-Allow-Origin", "*");
resp->addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS,DELETE");
```

## Verification Steps

### 1. Verify Backend Startup

Run backend in terminal and look for:
```
=== Backend Started: 2026-05-01 14:30:00 ===
Storage folders initialized
Drogon app configured
```

### 2. Verify Storage Folders

After backend runs, check:
```bash
ls -la storage/
# Should show: logs/, uploads/, extracted/, index/
```

### 3. Verify Log File

```bash
cat storage/logs/backend.log
# Should show startup messages
```

### 4. Test Upload Directly

Using curl (Windows PowerShell):
```powershell
$file = "mock-backend/test_upload.csv"
$uri = "http://localhost:8080/upload"
curl -Form "file=@$file" $uri
```

Expected response:
```json
{
  "success": true,
  "message": "File uploaded and processed successfully.",
  "filename": "test_upload.csv",
  "file_type": "csv",
  "status": "processed",
  "pipeline": "structured",
  "metadata_id": 1
}
```

### 5. Test GET /datasets

```powershell
curl http://localhost:8080/datasets
```

Expected response: Array of dataset objects with id, file_name, status, etc.

## Port Configuration

**Current Setup:**
- Backend runs on: `8080` (in main.cpp: `HttpServer::start(8080)`)
- Frontend expects: `8081` (in App.jsx: `API_BASE = 'http://localhost:8081'`)

**Options:**
1. **Change backend to 8081**: Edit `core/src/main.cpp` line with `start(8080)` → `start(8081)`
2. **Change frontend to 8080**: Edit `frontend/src/App.jsx` line with `8081` → `8080`
3. **Set up reverse proxy**: Proxy 8081 → 8080

## Check Compilation Errors

Look for these files that were added:
- `core/src/core/OllamaHealthChecker.cpp` ✓
- `core/src/core/FallbackResponseComposer.cpp` ✓
- `core/src/core/Logger.cpp` ✓

If build fails, these might not be compiling. Check:
```bash
cd core/build
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON
make 2>&1 | grep error
```

## Emergency Reset

If everything seems broken:

1. Clean build:
```bash
cd core
rm -rf build
mkdir -p build
cd build
cmake ..
make
```

2. Clean storage:
```bash
rm -rf storage/uploads storage/extracted storage/logs storage/metadata.db
```

3. Restart backend

4. Try upload again

## Still Having Issues?

If none of the above work, check:

1. **Port already in use**: `netstat -ano | findstr 8080`
2. **Firewall blocking port**: Try connecting from same machine
3. **Build incomplete**: Check for compilation errors above
4. **Old binary running**: Kill existing process and rebuild
5. **Wrong working directory**: Backend should run from project root

## Contact Points

Key files to verify are set up correctly:
- `core/CMakeLists.txt` - Lists all source files
- `core/include/server/UploadController.hpp` - Endpoint registration
- `core/src/server/UploadController.cpp` - Upload handler
- `frontend/src/App.jsx` - API base URL
