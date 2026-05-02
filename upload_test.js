const fs = require('fs');
const FormData = require('form-data');
const axios = require('axios');

async function uploadFile() {
  const form = new FormData();
  form.append('file', fs.createReadStream('c:\\Users\\rithi\\OneDrive\\Documents\\vetalmed project\\mock-backend\\test_upload.csv'));

  try {
    const response = await axios.post('http://localhost:3003/upload', form, {
      headers: form.getHeaders(),
    });
    console.log('Upload successful:', response.data);
  } catch (error) {
    console.error('Upload failed:', error.message);
  }
}

uploadFile();