#!/usr/bin/env bash
# Debug and test VetalMed Backend Setup

echo "=== VetalMed Backend Debugging ==="
echo ""

# 1. Check if backend is running
echo "1. Checking if backend is running on port 8080..."
curl -s http://localhost:8080/health > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   ✓ Backend is running on port 8080"
    echo "   Response:"
    curl -s http://localhost:8080/health | jq '.' 2>/dev/null || curl -s http://localhost:8080/health
else
    echo "   ✗ Backend is NOT running on port 8080"
    echo ""
    echo "   To build and run backend:"
    echo "   cd core"
    echo "   mkdir -p build"
    echo "   cd build"
    echo "   cmake .."
    echo "   make"
    echo "   ./erp_ai_backend"
fi

echo ""

# 2. Check if backend is running on port 8081 (alternate)
echo "2. Checking if backend is running on port 8081..."
curl -s http://localhost:8081/health > /dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "   ✓ Backend is running on port 8081"
    echo "   NOTE: Frontend expects port 8081, this is correct!"
else
    echo "   ✗ Backend is NOT running on port 8081"
fi

echo ""

# 3. Check storage folders
echo "3. Checking storage folders..."
if [ -d "storage/uploads" ]; then
    echo "   ✓ storage/uploads exists"
else
    echo "   ✗ storage/uploads missing - will be created on first upload"
fi

if [ -d "storage/extracted" ]; then
    echo "   ✓ storage/extracted exists"
else
    echo "   ✗ storage/extracted missing - will be created on first upload"
fi

if [ -d "storage/logs" ]; then
    echo "   ✓ storage/logs exists"
    echo "   Recent logs:"
    tail -5 storage/logs/backend.log 2>/dev/null || echo "   (no logs yet)"
else
    echo "   ✗ storage/logs missing - will be created on backend startup"
fi

echo ""

# 4. Test upload endpoint
echo "4. Testing upload endpoint..."
if [ -f "mock-backend/test_upload.csv" ]; then
    echo "   Uploading test file..."
    curl -X POST -F "file=@mock-backend/test_upload.csv" http://localhost:8080/upload 2>/dev/null | jq '.' 2>/dev/null
    echo ""
else
    echo "   ✗ Test file not found"
fi

echo ""

# 5. Check datasets
echo "5. Checking datasets..."
curl -s http://localhost:8080/datasets 2>/dev/null | jq '.' 2>/dev/null
if [ $? -ne 0 ]; then
    echo "   ✗ Failed to fetch datasets (backend not running?)"
fi

echo ""
echo "=== End of Debug Report ==="
