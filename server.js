// server.js
require('dotenv').config();
const express = require('express');
const mongoose = require('mongoose');
const cors = require('cors');
const bodyParser = require('body-parser');
const path = require('path'); // Add path module for file serving

// Initialize app once
const app = express();

// Middleware
app.use(cors());
app.use(bodyParser.json());

// MongoDB Connection (use your actual URI)
const MONGODB_URI = process.env.MONGODB_URI || 'mongodb+srv://rishikesh7115:rksonu%40420@cluster0.epmfrju.mongodb.net/health_Moniter?retryWrites=true&w=majority';

mongoose.connect(MONGODB_URI)
  .then(() => console.log('Connected to MongoDB Atlas'))
  .catch(err => console.error('MongoDB connection error:', err));

// Define schema and model once
const SensorDataSchema = new mongoose.Schema({
  heartRate: { type: Number, required: true },
  spo2: { type: Number, required: true },
  temperature: { type: Number, required: true },
  humidity: { type: Number, required: true },
  timestamp: { type: Date, default: Date.now }
});

const SensorData = mongoose.model('SensorData', SensorDataSchema);

// Routes
app.post('/api/data', async (req, res) => {
  try {
    const { heartRate, spo2, temperature, humidity } = req.body;
    
    if (!heartRate || !spo2 || !temperature || !humidity) {
      return res.status(400).json({ error: 'Missing required fields' });
    }

    const newData = new SensorData({ 
      heartRate, 
      spo2, 
      temperature, 
      humidity 
    });

    await newData.save();
    res.status(201).json({ 
      success: true,
      message: 'Data saved successfully',
      data: newData
    });
  } catch (error) {
    console.error('Error saving data:', error);
    res.status(500).json({ 
      success: false,
      error: 'Failed to save data',
      details: error.message 
    });
  }
});

app.get('/api/data', async (req, res) => {
  try {
    const limit = parseInt(req.query.limit) || 50;
    const data = await SensorData.find()
      .sort({ timestamp: -1 })
      .limit(limit);

    res.json({
      success: true,
      count: data.length,
      data
    });
  } catch (error) {
    console.error('Error fetching data:', error);
    res.status(500).json({ 
      success: false,
      error: 'Failed to fetch data',
      details: error.message 
    });
  }
});

// Health check
app.get('/api/health', (req, res) => {
  res.json({ 
    status: 'OK',
    database: mongoose.connection.readyState === 1 ? 'Connected' : 'Disconnected',
    timestamp: new Date() 
  });
});

// Serve static files from 'public' directory (if you have frontend files)
app.use(express.static('public'));

// Handle root route
app.get('/', (req, res) => {
  res.send('Health Monitor Backend is running!');
  // OR serve your frontend HTML:
  // res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Error handling
app.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).json({
    success: false,
    error: 'Internal server error',
    message: err.message
  });
});

// Start server
const PORT = process.env.PORT || 5000;
app.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});
