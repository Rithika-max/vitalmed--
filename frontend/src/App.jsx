import { useEffect, useRef, useState } from 'react';
import axios from 'axios';
import { API_BASE_URL } from './api';

function App() {
  const [selectedFile, setSelectedFile] = useState(null);
  const [uploadMessage, setUploadMessage] = useState('');
  const [datasets, setDatasets] = useState([]);
  const [chatQuery, setChatQuery] = useState('');
  const [messages, setMessages] = useState([]);
  const [loading, setLoading] = useState(false);
  const [backendConnected, setBackendConnected] = useState(false);
  const [backendMessage, setBackendMessage] = useState('Checking backend connection...');
  const retryTimerRef = useRef(null);
  const [hasEverConnected, setHasEverConnected] = useState(false);

  useEffect(() => {
    checkBackendHealth();
    return () => {
      if (retryTimerRef.current) {
        clearInterval(retryTimerRef.current);
      }
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const startRetryLoop = () => {
    if (retryTimerRef.current) {
      return;
    }
    retryTimerRef.current = setInterval(() => {
      checkBackendHealth();
    }, 5000);
  };

  const checkBackendHealth = async () => {
    try {
      const response = await axios.get(`${API_BASE_URL}/health`, { timeout: 3000 });
      if (response.data?.backend === 'running') {
        setBackendConnected(true);
        setBackendMessage('Backend Connected');
        if (!hasEverConnected) {
          fetchDatasets();
          setHasEverConnected(true);
        }
        if (retryTimerRef.current) {
          clearInterval(retryTimerRef.current);
          retryTimerRef.current = null;
        }
        return;
      }
      throw new Error('Unexpected health response');
    } catch (error) {
      setBackendConnected(false);
      setBackendMessage('Backend is not running. Please start the project using start_project.bat.');
      startRetryLoop();
      console.warn('Backend health check failed', error);
    }
  };

  const fetchDatasets = async () => {
    try {
      const response = await axios.get(`${API_BASE_URL}/datasets`);
      setDatasets(response.data);
    } catch (error) {
      console.warn('Unable to fetch datasets:', error);
    }
  };

  const handleUpload = async () => {
    if (!selectedFile) {
      setUploadMessage('Please select a file first.');
      return;
    }
    const formData = new FormData();
    formData.append('file', selectedFile);
    try {
      const response = await axios.post(`${API_BASE_URL}/upload`, formData);
      setUploadMessage(response.data.message || 'Upload successful.');
      fetchDatasets();
    } catch (error) {
      const serverMessage = error?.response?.data?.message;
      setUploadMessage(serverMessage ? `Upload failed: ${serverMessage}` : 'Upload failed.');
      console.error(error);
    }
  };

  const sendChat = async () => {
    if (!chatQuery.trim()) return;
    setLoading(true);
    const newMessages = [...messages, { role: 'user', text: chatQuery }];
    setMessages(newMessages);
    try {
      const response = await axios.post(`${API_BASE_URL}/chat`, { query: chatQuery, session_id: 'default' });
      const assistantText = response.data.summary || 'No response';
      setMessages([...newMessages, { role: 'assistant', text: assistantText }]);
    } catch (error) {
      console.error(error);
      setMessages([...newMessages, { role: 'assistant', text: 'Unable to get answer.' }]);
    } finally {
      setLoading(false);
      setChatQuery('');
    }
  };

  const handleKeyDown = (event) => {
    if (event.ctrlKey && event.key === 'Enter') {
      sendChat();
    }
  };

  const handleDelete = async (fileId, fileName) => {
    if (!window.confirm(`Are you sure you want to delete "${fileName}"?`)) {
      return;
    }

    try {
      await axios.delete(`${API_BASE_URL}/datasets/${fileId}`);
      setUploadMessage(`File "${fileName}" deleted successfully.`);
      fetchDatasets();
      setTimeout(() => setUploadMessage(''), 3000);
    } catch (error) {
      const errorMessage = error?.response?.data?.message || 'Failed to delete file.';
      setUploadMessage(`Delete failed: ${errorMessage}`);
      console.error(error);
    }
  };

  return (
    <div className="min-h-screen bg-slate-100 p-4">
      <div className="mx-auto flex max-w-[1400px] gap-4">
        <div className="flex-1 rounded-3xl bg-white p-6 shadow-lg">
          <div className="flex items-center justify-between gap-4">
            <h1 className="text-2xl font-semibold text-slate-900">ERP Demo Workspace</h1>
            <span className={backendConnected ? 'rounded-full bg-emerald-100 px-4 py-2 text-emerald-700' : 'rounded-full bg-amber-100 px-4 py-2 text-amber-700'}>
              {backendMessage}
            </span>
          </div>
          <div className="mt-6 space-y-6">
            <div className="rounded-2xl border border-slate-200 p-5">
              <h2 className="text-xl font-medium">Upload & Auto-Process</h2>
              <div className="mt-4 flex flex-col gap-3">
                <input type="file" onChange={(e) => setSelectedFile(e.target.files?.[0] || null)} />
                {selectedFile && <p className="text-sm text-slate-600">Selected: {selectedFile.name}</p>}
                <button className="rounded-full bg-slate-900 px-5 py-2 text-white hover:bg-slate-700" onClick={handleUpload}>Upload / Process</button>
                <p className="text-sm text-slate-500">{uploadMessage}</p>
              </div>
            </div>
            <div className="rounded-2xl border border-slate-200 p-5">
              <h2 className="text-xl font-medium">Uploaded Files</h2>
              <div className="mt-4 grid gap-3">
                {datasets.length === 0 ? (
                  <p className="text-slate-500">No uploaded datasets yet.</p>
                ) : (
                  datasets.map((dataset) => (
                    <div key={dataset.id} className="rounded-2xl border border-slate-100 bg-slate-50 p-4 flex items-center justify-between">
                      <div className="flex-1">
                        <p className="font-semibold">{dataset.file_name}</p>
                        <p className="text-sm text-slate-600">{dataset.file_type} · {dataset.uploaded_at}</p>
                      </div>
                      <button
                        onClick={() => handleDelete(dataset.id, dataset.file_name)}
                        className="ml-4 rounded-full bg-red-500 px-3 py-1 text-sm text-white hover:bg-red-600 transition-colors"
                      >
                        Delete
                      </button>
                    </div>
                  ))
                )}
              </div>
            </div>
          </div>
        </div>

        <div className="w-[420px] rounded-3xl bg-slate-950 p-6 text-white shadow-lg flex flex-col">
          <h2 className="text-2xl font-semibold">AI Assistant</h2>
          <div className="mt-5 flex-1 flex flex-col gap-4 overflow-hidden rounded-3xl bg-slate-900 p-4 shadow-inner">
            <div className="flex-1 flex flex-col gap-3 overflow-y-auto pr-2">
              {messages.length === 0 ? (
                <p className="text-slate-400">Ask your ERP assistant anything. It will use uploaded data and ERP APIs only.</p>
              ) : (
                messages.map((message, index) => (
                  <div key={index} className={message.role === 'user' ? 'self-end rounded-2xl bg-slate-700 px-4 py-3 max-w-xs' : 'self-start rounded-2xl bg-slate-800 px-4 py-3 max-w-xs'}>
                    <p className="text-sm leading-6">{message.text}</p>
                  </div>
                ))
              )}
            </div>
            <div className="rounded-2xl border border-slate-700 bg-slate-950 p-4 flex-shrink-0">
              <label className="block text-sm text-slate-400">Query</label>
              <textarea
                value={chatQuery}
                onChange={(e) => setChatQuery(e.target.value)}
                onKeyDown={handleKeyDown}
                className="mt-2 h-24 w-full rounded-2xl border border-slate-800 bg-slate-900 p-3 text-slate-100 outline-none focus:border-slate-500"
                placeholder="Type your question and press Ctrl+Enter"
              />
              <button
                onClick={sendChat}
                disabled={loading}
                className="mt-3 rounded-full bg-sky-500 px-5 py-2 text-sm font-semibold text-slate-950 hover:bg-sky-400 disabled:opacity-60"
              >
                {loading ? 'Thinking...' : 'Send'}
              </button>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;
