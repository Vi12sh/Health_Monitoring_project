

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
      { label: 'Heart Rate', data: [], borderColor: '#e74c3c', tension: 0.1 },
      { label: 'SpO2', data: [], borderColor: '#3498db', tension: 0.1 },
      { label: 'Temperature', data: [], borderColor: '#f39c12', tension: 0.1 },
      { label: 'Humidity', data: [], borderColor: '#2ecc71', tension: 0.1 }
    ]
  },
  options: {
    responsive: true,
    scales: { y: { beginAtZero: false } }
  }
});

// Fetch Data from Backend
async function fetchData() {
  try {
    const response = await fetch('/api/data');
    const { data } = await response.json();
    updateDashboard(data);
  } catch (error) {
    console.error('Failed to fetch data:', error);
  }
}

// Update UI with Data
function updateDashboard(data) {
  if (!data || data.length === 0) return;

  const latest = data[0];
  heartRateEl.textContent = latest.heartRate.toFixed(1);
  spo2El.textContent = latest.spo2.toFixed(1);
  tempEl.textContent = latest.temperature.toFixed(1);
  humidityEl.textContent = latest.humidity.toFixed(1);

  // Update Chart
  const timeLabels = data.map(d => new Date(d.timestamp).toLocaleTimeString());
  chart.data.labels = timeLabels.reverse();
  chart.data.datasets[0].data = data.map(d => d.heartRate).reverse();
  chart.data.datasets[1].data = data.map(d => d.spo2).reverse();
  chart.data.datasets[2].data = data.map(d => d.temperature).reverse();
  chart.data.datasets[3].data = data.map(d => d.humidity).reverse();
  chart.update();
}

// Refresh data every 5 seconds
fetchData();
setInterval(fetchData, 5000);