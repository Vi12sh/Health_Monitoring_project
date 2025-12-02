// Access the canvas element
const ctx = document.getElementById('healthChart').getContext('2d');

// Create a new chart
const healthChart = new Chart(ctx, {
    type: 'line', // Line chart type
    data: {
        labels: ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'], // Days of the week
        datasets: [{
            label: 'Heart Rate',
            data: [78, 80, 76, 79, 82, 85], // Sample heart rate data
            borderColor: 'red', // Line color
            backgroundColor: 'rgba(255, 99, 132, 0.2)', // Fill color
            borderWidth: 2,
            tension: 0.4, // Smoothness of the curve
        },
        {
            label: 'SpO2',
            data: [96, 95, 97, 95, 96, 94], // Sample SpO2 data
            borderColor: 'blue', // Line color
            backgroundColor: 'rgba(54, 162, 235, 0.2)', // Fill color
            borderWidth: 2,
            tension: 0.4, // Smoothness of the curve
        }]
    },
    options: {
        responsive: true, // Make it responsive to screen size
        plugins: {
            legend: {
                position: 'top', // Position the legend at the top
            }
        },
        scales: {
            y: {
                beginAtZero: true, // Ensure Y axis starts from 0
            }
        }
    }
});
