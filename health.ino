#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi credentials
const char* ssid = "LENOVO"; // Your 2.4 GHz Wi-Fi SSID
const char* password = "90000000"; // Your Wi-Fi password

// Backend server details
const char* backendUrl = "http://192.168.137.10:5000/api/data"; // Replace with actual Node.js server IP

// Define pins
#define DHTPIN 23        // GPIO23 for DHT22 data pin
#define DHTTYPE DHT22   // Specify DHT22 sensor
#define SDA_PIN 27      // Custom SDA pin for LCD
#define SCL_PIN 26      // Custom SCL pin for LCD

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Initialize LCD (0x27 is common I2C address, 16x2 display)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Initialize Web Server and WebSocket
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Function prototype for WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

// Frontend files (embedded as strings)
const char* index_html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Health Monitor Dashboard</title>
  <link rel="stylesheet" href="/styles.css">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.4/css/all.min.css">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
  <nav class="navbar">
    <div class="nav-logo">Health Monitor</div>
    <ul class="nav-links">
      <li><a href="/" class="active">Home</a></li>
      <li><a href="/dashboard.html">Dashboard</a></li>
      <li><a href="/about.html">About</a></li>
      <li><a href="/contact.html">Contact</a></li>
    </ul>
  </nav>
  <section class="hero">
    <h1>Stay Ahead with Real-Time Health Monitoring</h1>
    <p>Track your Heart Rate, Oxygen Level, Temperature, and Humidity with our intelligent monitoring system.</p>
    <a href="/dashboard.html" class="btn">View Live Dashboard</a>
  </section>
  <section class="features">
    <h2>Why Choose Our Health Monitoring System?</h2>
    <div class="feature-cards">
      <div class="feature-card">
        <img src="https://cdn-icons-png.flaticon.com/512/2965/2965567.png" alt="Heart Monitoring" />
        <h3>Heart Monitoring</h3>
        <p>Accurate real-time heart rate analysis with early warning alerts.</p>
      </div>
      <div class="feature-card">
        <img src="https://cdn-icons-png.flaticon.com/512/1792/1792930.png" alt="Oxygen Monitoring" />
        <h3>SpO2 Tracking</h3>
        <p>Continuous blood oxygen tracking to keep you informed every second.</p>
      </div>
      <div class="feature-card">
        <img src="https://cdn-icons-png.flaticon.com/512/2942/2942911.png" alt="Temperature" />
        <h3>Temperature Sensing</h3>
        <p>Detects your body or room temperature instantly and accurately.</p>
      </div>
      <div class="feature-card">
        <img src="https://cdn-icons-png.flaticon.com/512/4140/4140047.png" alt="Humidity Control" />
        <h3>Humidity Monitoring</h3>
        <p>Perfect for patients needing controlled air humidity environments.</p>
      </div>
    </div>
  </section>
  <section class="overview-simple">
    <h2>Health Data Overview</h2>
    <div class="overview-item">
      <label>Heart Rate</label>
      <div class="progress-bar">
        <div class="progress" style="width: 75%; background-color: red;">75 bpm</div>
      </div>
    </div>
    <div class="overview-item">
      <label>SpO2</label>
      <div class="progress-bar">
        <div class="progress" style="width: 92%; background-color: blue;">92%</div>
      </div>
    </div>
    <div class="overview-item">
      <label>Temperature</label>
      <div class="progress-bar">
        <div class="progress" style="width: 38%; background-color: orange;">38°C</div>
      </div>
    </div>
    <div class="overview-item">
      <label>Humidity</label>
      <div class="progress-bar">
        <div class="progress" style="width: 50%; background-color: green;">50%</div>
      </div>
    </div>
  </section>
  <div class="footer">
    <p>© 2025 Health Monitor Dashboard. All rights reserved.</p>
  </div>
</body>
</html>
)rawliteral";

const char* dashboard_html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Health Monitoring Dashboard</title>
  <link rel="stylesheet" href="/dashboard.css">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.4/css/all.min.css">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
  <nav class="navbar">
    <div class="nav-logo">Health Monitor</div>
    <ul class="nav-links">
      <li><a href="/">Home</a></li>
      <li><a href="/dashboard.html" class="active">Dashboard</a></li>
      <li><a href="/about.html">About</a></li>
      <li><a href="/contact.html">Contact</a></li>
    </ul>
  </nav>
  <div class="dashboard">
    <h1>Health Monitoring Dashboard</h1>
    <div class="metrics">
      <div class="metric-card heart">
        <h2>Heart Rate</h2>
        <div id="heartRate">--</div>
        <span>bpm</span>
      </div>
      <div class="metric-card spo2">
        <h2>SpO2</h2>
        <div id="spo2">--</div>
        <span>%</span>
      </div>
      <div class="metric-card temp">
        <h2>Temperature</h2>
        <div id="temperature">--</div>
        <span>°C</span>
      </div>
      <div class="metric-card humidity">
        <h2>Humidity</h2>
        <div id="humidity">--</div>
        <span>%</span>
      </div>
    </div>
    <div class="chart-container">
      <canvas id="healthChart"></canvas>
    </div>
  </div>
  <div class="footer">
    <p>© 2025 Health Monitor Dashboard. All rights reserved.</p>
  </div>
  <script src="/script.js"></script>
</body>
</html>
)rawliteral";

const char* styles_css = R"rawliteral(
/* Common Styles: Navbar + Footer */
body {
  font-family: 'Poppins', sans-serif;
  margin: 0;
  padding: 0;
  background-color: #f9f9f9;
}

.navbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background-color: #0077cc;
  padding: 15px 30px;
}

.nav-logo {
  font-size: 1.5rem;
  font-weight: bold;
  color: #fff;
}

.nav-links {
  list-style: none;
  display: flex;
  gap: 20px;
}

.nav-links li a {
  color: white;
  text-decoration: none;
  font-weight: 500;
}

.nav-links li a.active,
.nav-links li a:hover {
  border-bottom: 2px solid #fff;
}

.hero {
  height: 90vh;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  text-align: center;
  background: linear-gradient(to right, #4facfe, #00f2fe);
  color: white;
  padding: 20px;
}

.hero h1 {
  font-size: 3rem;
  margin-bottom: 20px;
}

.hero p {
  font-size: 1.2rem;
  margin-bottom: 30px;
}

.btn {
  padding: 10px 20px;
  background-color: white;
  color: #0077cc;
  border: none;
  border-radius: 5px;
  text-decoration: none;
  font-weight: bold;
  transition: background-color 0.3s ease;
}

.btn:hover {
  background-color: #f0f0f0;
}

.features {
  padding: 60px 20px;
  text-align: center;
  background: #f7f9fc;
}

.features h2 {
  font-size: 2.5rem;
  margin-bottom: 40px;
}

.feature-cards {
  display: flex;
  flex-wrap: wrap;
  gap: 30px;
  justify-content: center;
}

.feature-card {
  background: white;
  padding: 20px;
  width: 250px;
  border-radius: 10px;
  box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
  transition: transform 0.3s ease;
}

.feature-card:hover {
  transform: translateY(-10px);
}

.feature-card img {
  width: 80px;
  height: 80px;
  object-fit: contain;
  margin-bottom: 15px;
}

.feature-card h3 {
  font-size: 1.4rem;
  margin-bottom: 10px;
}

.feature-card p {
  font-size: 1rem;
  color: #555;
}

.overview-simple {
  max-width: 800px;
  margin: 40px auto;
  padding: 20px;
  background: #f5f5f5;
  border-radius: 10px;
  text-align: center;
}

.overview-simple h2 {
  margin-bottom: 30px;
  color: #333;
}

.overview-item {
  margin-bottom: 20px;
}

.overview-item label {
  display: block;
  margin-bottom: 8px;
  font-weight: bold;
  color: #555;
}

.progress-bar {
  width: 100%;
  background: #ddd;
  border-radius: 20px;
  overflow: hidden;
  height: 25px;
}

.progress {
  height: 100%;
  text-align: center;
  color: #fff;
  line-height: 25px;
  font-avsize: 14px;
}

.footer {
  text-align: center;
  margin-top: 40px;
  padding: 20px;
  background-color: #333;
  color: white;
  border-radius: 10px 10px 0 0;
}

@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(30px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 768px) {
  .nav-links {
    display: none;
  }
  .hero {
    height: auto;
    padding: 40px 20px;
  }
  .hero h1 {
    font-size: 2rem;
  }
  .features {
    padding: 40px 20px;
  }
  .feature-cards {
    flex-direction: column;
    align-items: center;
  }
  .feature-card {
    width: 100%;
    max-width: 300px;
  }
}
)rawliteral";

const char* dashboard_css = R"rawliteral(
body {
  font-family: 'Poppins', sans-serif;
  margin: 0;
  padding: 0;
  background-color: #f9f9f9;
}

.navbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background-color: #0077cc;
  padding: 15px 30px;
}

.nav-logo {
  font-size: 1.5rem;
  font-weight: bold;
  color: #fff;
}

.nav-links {
  list-style: none;
  display: flex;
  gap: 20px;
}

.nav-links li a {
  color: white;
  text-decoration: none;
  font-weight: 500;
}

.nav-links li a.active,
.nav-links li a:hover {
  border-bottom: 2px solid #fff;
}

.dashboard {
  max-width: 1200px;
  margin: 50px auto;
  padding: 30px;
  background-color: #ffffff;
  border-radius: 20px;
  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.1);
  animation: fadeIn 0.8s ease;
}

.dashboard h1 {
  text-align: center;
  font-size: 2.5rem;
  color: #333;
  margin-bottom: 40px;
}

.metrics {
  display: flex;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 20px;
}

.metric-card {
  flex: 1 1 calc(25% - 20px);
  padding: 25px;
  border-radius: 20px;
  background: linear-gradient(135deg, #000, #e0e0e0);
  text-align: center;
  color: #333;
  position: relative;
  overflow: hidden;
  transition: all 0.4s ease;
  box-shadow: 0 8px 20px rgba(0, 0, 0, 0.1);
}

.metric-card:hover {
  transform: translateY(-10px);
  box-shadow: 0 12px 30px rgba(0, 0, 0, 0.2);
}

.metric-card.heart {
  background: linear-gradient(135deg, #ff758c, #ff7eb3);
}

.metric-card.spo2 {
  background: linear-gradient(135deg, #74ebd5, #acb6e5);
}

.metric-card.temp {
  background: linear-gradient(135deg, #f6d365, #fda085);
}

.metric-card.humidity {
  background: linear-gradient(135deg, #89f7fe, #66a6ff);
}

.metric-card h2 {
  font-size: 1.5rem;
  margin-bottom: 15px;
  color: #fff;
}

.metric-card div {
  font-size: 2.5rem;
  font-weight: bold;
  margin-bottom: 5px;
  color: #fff;
}

.metric-card span {
  font-size: 1rem;
  color: #f0f0f0;
}

.metric-card::before {
  content: '';
  font-family: 'Font Awesome 5 Free';
  font-weight: 900;
  font-size: 3rem;
  position: absolute;
  top: 20px;
  right: 20px;
  opacity: 0.2;
}

.metric-card.heart::before {
  content: "\f004";
}

.metric-card.spo2::before {
  content: "\f5c9";
}

.metric-card.temp::before {
  content: "\f2c9";
}

.metric-card.humidity::before {
  content: "\f043";
}

.chart-container {
  margin-top: 50px;
  height: 400px;
}

.footer {
  text-align: center;
  margin-top: 40px;
  padding: 20px;
  background-color: #333;
  color: white;
  border-radius: 10px 10px 0 0;
}

@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(30px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 768px) {
  .metrics {
    flex-direction: column;
    align-items: center;
  }
  .metric-card {
    flex: 1 1 80%;
  }
}
)rawliteral";

const char* about_html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>About Us - Health Monitor</title>
  <link rel="stylesheet" href="/about.css">
</head>
<body>
  <nav class="navbar">
    <div class="nav-logo">Health Monitor</div>
    <ul class="nav-links">
      <li><a href="/">Home</a></li>
      <li><a href="/dashboard.html">Dashboard</a></li>
      <li><a href="/about.html" class="active">About</a></li>
      <li><a href="/contact.html">Contact</a></li>
    </ul>
  </nav>
  <section class="about">
    <h1>About Us</h1>
    <p>We are committed to providing real-time health monitoring solutions for individuals and healthcare providers.</p>
    <p>Our mission is to empower people with technology to stay informed about their vital health parameters and lead healthier lives.</p>
  </section>
  <footer class="footer">
    <p>© 2025 Health Monitor Dashboard. All rights reserved.</p>
  </footer>
</body>
</html>
)rawliteral";

const char* about_css = R"rawliteral(
body {
  font-family: 'Poppins', sans-serif;
  margin: 0;
  padding: 0;
  background-color: #f9f9f9;
}

.navbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background-color: #0077cc;
  padding: 15px 30px;
}

.nav-logo {
  font-size: 1.5rem;
  font-weight: bold;
  color: #fff;
}

.nav-links {
  list-style: none;
  display: flex;
  gap: 20px;
}

.nav-links li a {
  color: white;
  text-decoration: none;
  font-weight: 500;
}

.nav-links li a.active,
.nav-links li a:hover {
  border-bottom: 2px solid #fff;
}

.about {
  padding: 50px;
  text-align: center;
}

.about h1 {
  font-size: 2.5rem;
  margin-bottom: 20px;
}

.about p {
  font-size: 1.2rem;
  max-width: 800px;
  margin: auto;
}

.footer {
  text-align: center;
  margin-top: 40px;
  padding: 20px;
  background-color: #333;
  color: white;
  border-radius: 10px 10px 0 0;
}

@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(30px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 768px) {
  .metrics {
    flex-direction: column;
    align-items: center;
  }
  .metric-card {
    flex: 1 1 80%;
  }
}
)rawliteral";

const char* contact_html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Contact Us - Health Monitor</title>
  <link rel="stylesheet" href="/contact.css">
</head>
<body>
  <nav class="navbar">
    <div class="nav-logo">Health Monitor</div>
    <ul class="nav-links">
      <li><a href="/">Home</a></li>
      <li><a href="/dashboard.html">Dashboard</a></li>
      <li><a href="/about.html">About</a></li>
      <li><a href="/contact.html" class="active">Contact</a></li>
    </ul>
  </nav>
  <section class="contact">
    <h1>Contact Us</h1>
    <form class="contact-form">
      <input type="text" placeholder="Your Name" required>
      <input type="email" placeholder="Your Email" required>
      <textarea placeholder="Your Message" rows="5" required></textarea>
      <button type="submit" class="btn">Send Message</button>
    </form>
  </section>
  <footer class="footer">
    <p>© 2025 Health Monitor Dashboard. All rights reserved.</p>
  </footer>
</body>
</html>
)rawliteral";

const char* contact_css = R"rawliteral(
body {
  font-family: 'Poppins', sans-serif;
  margin: 0;
  padding: 0;
  background-color: #f9f9f9;
}

.navbar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background-color: #0077cc;
  padding: 15px 30px;
}

.nav-logo {
  font-size: 1.5rem;
  font-weight: bold;
  color: #fff;
}

.nav-links {
  list-style: none;
  display: flex;
  gap: 20px;
}

.nav-links li a {
  color: white;
  text-decoration: none;
  font-weight: 500;
}

.nav-links li a.active,
.nav-links li a:hover {
  border-bottom: 2px solid #fff;
}

.contact {
  padding: 50px;
  text-align: center;
}

.contact h1 {
  font-size: 2.5rem;
  margin-bottom: 20px;
}

.contact-form {
  max-width: 500px;
  margin: auto;
  display: flex;
  flex-direction: column;
  gap: 15px;
}

.contact-form input,
.contact-form textarea {
  width: 100%;
  padding: 12px;
  border: 1px solid #ccc;
  border-radius: 5px;
}

.contact-form button {
  background-color: #0077cc;
  color: white;
  padding: 10px;
  border: none;
  border-radius: 5px;
  font-weight: bold;
}

.contact-form button:hover {
  background-color: #005fa3;
}

.footer {
  text-align: center;
  margin-top: 40px;
  padding: 20px;
  background-color: #333;
  color: white;
  border-radius: 10px 10px 0 0;
}

@keyframes fadeIn {
  from {
    opacity: 0;
    transform: translateY(30px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 768px) {
  .metrics {
    flex-direction: column;
    align-items: center;
  }
  .metric-card {
    flex: 1 1 80%;
  }
}
)rawliteral";

const char* script_js = R"rawliteral(
// DOM Elements
const heartRateEl = document.getElementById('heartRate');
const spo2El = document.getElementById('spo2');
const tempEl = document.getElementById('temperature');
const humidityEl = document.getElementById('humidity');

// Initialize Chart
const ctx = document.getElementById('healthChart').getContext('2d');
const chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      { label: 'Heart Rate', data: [], borderColor: '#e74c3c', tension: 0.4, borderWidth: 2, pointRadius: 3 },
      { label: 'SpO2', data: [], borderColor: '#3498db', tension: 0.4, borderWidth: 2, pointRadius: 3 },
      { label: 'Temperature', data: [], borderColor: '#f39c12', tension: 0.4, borderWidth: 2, pointRadius: 3 },
      { label: 'Humidity', data: [], borderColor: '#2ecc71', tension: 0.4, borderWidth: 2, pointRadius: 3 }
    ]
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    scales: {
      y: { beginAtZero: false },
      x: { title: { display: true, text: 'Time' } }
    }
  }
});

// WebSocket Connection
let socket;
const healthData = [];
const backendUrl = 'http://192.168.137.10:5000'; // Replace with actual Node.js server IP

function connectWebSocket() {
  socket = new WebSocket('ws://192.168.137.100:81/'); // Hardcoded IP for debugging
  socket.onopen = () => {
    console.log('WebSocket connected');
  };
  socket.onmessage = (event) => {
    console.log('Raw data:', event.data);
    try {
      const data = JSON.parse(event.data);
      console.log('Parsed data:', data);
      heartRateEl.textContent = data.heartRate.toFixed(1);
      spo2El.textContent = data.spo2.toFixed(1);
      tempEl.textContent = data.temperature.toFixed(1);
      humidityEl.textContent = data.humidity.toFixed(1);
      healthData.push({
        time: new Date().toLocaleTimeString(),
        heartRate: data.heartRate,
        spo2: data.spo2,
        temperature: data.temperature,
        humidity: data.humidity
      });
      console.log('Health data:', healthData);
      if (healthData.length > 20) healthData.shift();
      chart.data.labels = healthData.map(entry => entry.time);
      chart.data.datasets[0].data = healthData.map(entry => entry.heartRate);
      chart.data.datasets[1].data = healthData.map(entry => entry.spo2);
      chart.data.datasets[2].data = healthData.map(entry => entry.temperature);
      chart.data.datasets[3].data = healthData.map(entry => entry.humidity);
      chart.update();
    } catch (e) {
      console.error('JSON parse error:', e);
    }
  };
  socket.onclose = () => {
    console.log('WebSocket disconnected. Reconnecting in 3 seconds...');
    setTimeout(connectWebSocket, 3000);
  };
  socket.onerror = (error) => {
    console.error('WebSocket error:', error);
  };
}

// Fetch historical data from backend
async function fetchHistoricalData() {
  try {
    const response = await fetch(`${backendUrl}/api/data?limit=20`);
    const result = await response.json();
    if (result.success) {
      healthData.length = 0; // Clear current data
      result.data.reverse().forEach(entry => {
        healthData.push({
          time: new Date(entry.timestamp).toLocaleTimeString(),
          heartRate: entry.heartRate,
          spo2: entry.spo2,
          temperature: entry.temperature,
          humidity: entry.humidity
        });
      });
      chart.data.labels = healthData.map(entry => entry.time);
      chart.data.datasets[0].data = healthData.map(entry => entry.heartRate);
      chart.data.datasets[1].data = healthData.map(entry => entry.spo2);
      chart.data.datasets[2].data = healthData.map(entry => entry.temperature);
      chart.data.datasets[3].data = healthData.map(entry => entry.humidity);
      chart.update();
      if (healthData.length > 0) {
        heartRateEl.textContent = healthData[healthData.length - 1].heartRate.toFixed(1);
        spo2El.textContent = healthData[healthData.length - 1].spo2.toFixed(1);
        tempEl.textContent = healthData[healthData.length - 1].temperature.toFixed(1);
        humidityEl.textContent = healthData[healthData.length - 1].humidity.toFixed(1);
      }
    } else {
      console.error('Failed to fetch historical data:', result.error);
    }
  } catch (error) {
    console.error('Error fetching historical data:', error);
  }
}

// Add button for historical data
document.addEventListener('DOMContentLoaded', () => {
  const dashboard = document.querySelector('.dashboard');
  if (dashboard) {
    const button = document.createElement('button');
    button.textContent = 'Load Historical Data';
    button.style.margin = '20px auto';
    button.style.display = 'block';
    button.style.padding = '10px 20px';
    button.style.backgroundColor = '#0077cc';
    button.style.color = 'white';
    button.style.border = 'none';
    button.style.borderRadius = '5px';
    button.style.cursor = 'pointer';
    button.addEventListener('click', fetchHistoricalData);
    dashboard.insertBefore(button, dashboard.querySelector('.chart-container'));
  }
  connectWebSocket();
});
)rawliteral";

unsigned long lastUpdate = 0;
unsigned long lastLcdSwitch = 0;
bool showHeartSpo2 = true;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Health Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  Serial.println("Connecting to WiFi: " + String(ssid));
  WiFi.begin(ssid, password);
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(1000);
    Serial.print(".");
    wifiAttempts++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed. Retrying in 5 seconds...");
    delay(5000);
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    return;
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Access dashboard at: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  server.on("/", []() { server.send(200, "text/html", index_html); });
  server.on("/dashboard.html", []() { server.send(200, "text/html", dashboard_html); });
  server.on("/styles.css", []() { server.send(200, "text/css", styles_css); });
  server.on("/dashboard.css", []() { server.send(200, "text/css", dashboard_css); });
  server.on("/about.html", []() { server.send(200, "text/html", about_html); });
  server.on("/about.css", []() { server.send(200, "text/css", about_css); });
  server.on("/contact.html", []() { server.send(200, "text/html", contact_html); });
  server.on("/contact.css", []() { server.send(200, "text/css", contact_css); });
  server.on("/script.js", []() { server.send(200, "application/javascript", script_js); });
  server.onNotFound([]() { server.send(404, "text/plain", "Not Found"); });

  server.begin();
  Serial.println("HTTP server started on port 80");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started on port 81");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    int wifiAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifiAttempts < 10) {
      delay(1000);
      Serial.print(".");
      wifiAttempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nReconnected to WiFi");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("Access dashboard at: http://");
      Serial.print(WiFi.localIP());
      Serial.println("/");
    }
    return;
  }

  server.handleClient();
  webSocket.loop();
  Serial.println("WebSocket loop running"); // Debug print

  unsigned long currentMillis = millis();

  // Update LCD display every 3 seconds
  if (currentMillis - lastLcdSwitch >= 3000) {
    showHeartSpo2 = !showHeartSpo2;
    lastLcdSwitch = currentMillis;
  }

  // Update sensor data, WebSocket, and backend every 1 second
  if (currentMillis - lastUpdate >= 1000) {
    Serial.println("Updating data...");
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    float heartRate = 70.0 + random(-5, 5); // Mock heart rate (65–75 bpm)
    float spo2 = 95.0 + random(0, 50) / 10.0; // Mock SpO2 (95–100%)

    // Retry DHT read if failed
    int dhtRetries = 0;
    while ((isnan(humidity) || isnan(temperature)) && dhtRetries < 3) {
      Serial.println("Retrying DHT read...");
      delay(500);
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
      dhtRetries++;
    }
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("DHT read failed. Using mock data.");
      humidity = 60.0 + random(-5, 5);
      temperature = 25.0 + random(-2, 2);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("DHT Error!");
      delay(1000);
      lcd.clear();
    }

    // Print data to Serial Monitor
    Serial.print("Sending data - Humidity: ");
    Serial.print(humidity);
    Serial.print("%  Temperature: ");
    Serial.print(temperature);
    Serial.print("°C  Heart Rate: ");
    Serial.print(heartRate);
    Serial.print(" bpm  SpO2: ");
    Serial.print(spo2);
    Serial.println("%");

    // Update LCD
    lcd.clear();
    if (showHeartSpo2) {
      lcd.setCursor(0, 0);
      lcd.print("HR: ");
      lcd.print(heartRate, 1);
      lcd.print(" bpm");
      lcd.setCursor(0, 1);
      lcd.print("SpO2: ");
      lcd.print(spo2, 1);
      lcd.print("%");
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature, 1);
      lcd.print(" C");
      lcd.setCursor(0, 1);
      lcd.print("Hum: ");
      lcd.print(humidity, 1);
      lcd.print("%");
    }

    // Broadcast data via WebSocket
    String json = "{\"heartRate\":" + String(heartRate, 1) + ",\"spo2\":" + String(spo2, 1) +
                  ",\"temperature\":" + String(temperature, 1) + ",\"humidity\":" + String(humidity, 1) + "}";
    Serial.print("Broadcasting JSON: ");
    Serial.println(json);
    webSocket.broadcastTXT(json);
    Serial.println("Broadcast sent");

    // Send data to Node.js backend (optional, comment out if not needed)
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(backendUrl);
      http.addHeader("Content-Type", "application/json");
      Serial.print("Sending POST to: ");
      Serial.println(backendUrl);
      int httpResponseCode = http.POST(json);
      if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String response = http.getString();
        Serial.println("Response: " + response);
      } else {
        Serial.print("Error sending data to backend: ");
        Serial.println(httpResponseCode);
        Serial.println("Error details: " + http.errorToString(httpResponseCode));
      }
      http.end();
    }

    lastUpdate = currentMillis;
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("WebSocket [%u] Disconnected\n", num);
      break;
    case WStype_CONNECTED:
      Serial.printf("WebSocket [%u] Connected from URL: %s\n", num, payload);
      break;
    case WStype_TEXT:
      Serial.printf("WebSocket [%u] Received text: %s\n", num, payload);
      break;
    case WStype_PING:
      Serial.printf("WebSocket [%u] Received ping\n", num);
      break;
    case WStype_PONG:
      Serial.printf("WebSocket [%u] Received pong\n", num);
      break;
    default:
      Serial.printf("WebSocket [%u] Unknown event type: %d\n", num, type);
      break;
  }
}