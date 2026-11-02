/**
 * CineBook - Interactive Seat Selection Script
 */

let currentShowData = null;
let selectedSeats = new Map(); // seatId -> seatObject

async function loadShowAndSeats() {
  const urlParams = new URLSearchParams(window.location.search);
  const showId = urlParams.get('show_id');

  if (!showId) {
    showToast('No show selected. Redirecting to shows page...', 'error');
    setTimeout(() => window.location.href = 'shows.html', 1500);
    return;
  }

  try {
    const res = await fetch(`${API_BASE}/api/shows/${showId}/seats`);
    if (!res.ok) throw new Error('Show details not found in database');
    currentShowData = await res.json();

    renderShowHeader(currentShowData.show);
    renderSeatGrid(currentShowData.seats);
  } catch (err) {
    console.error(err);
    document.getElementById('seat-page-container').innerHTML = `
      <div class="card" style="text-align: center; padding: 3rem;">
        <h3 style="color: #ef4444; margin-bottom: 0.5rem;">Error Loading Show Seats</h3>
        <p style="color: var(--text-muted); margin-bottom: 1.5rem;">${err.message}</p>
        <a href="shows.html" class="btn btn-secondary">Back to Showtimes</a>
      </div>
    `;
  }
}

function renderShowHeader(show) {
  const container = document.getElementById('show-header-info');
  if (!container) return;

  container.innerHTML = `
    <div style="display: flex; gap: 1.5rem; align-items: center; flex-wrap: wrap;">
      <img src="${show.poster_url}" alt="${show.movie_title}" 
           style="width: 70px; height: 100px; object-fit: cover; border-radius: var(--radius-sm);" 
           onerror="this.onerror=null; this.src='https://images.unsplash.com/photo-1489599849927-2ee91cede3ba?w=600';" />
      <div>
        <h2 style="font-size: 1.6rem; margin-bottom: 0.2rem;">${show.movie_title}</h2>
        <p style="color: var(--text-muted); font-size: 0.95rem;">
          ${show.theatre_name} • <span style="color: var(--accent-cyan); font-weight: 600;">${show.screen_name}</span>
        </p>
        <p style="color: var(--text-dim); font-size: 0.85rem; margin-top: 0.2rem;">
          📅 <strong>${show.show_date}</strong> at ⏰ <strong>${show.show_time}</strong>
        </p>
      </div>
    </div>
  `;
}

function renderSeatGrid(seats) {
  const gridContainer = document.getElementById('seats-grid');
  if (!gridContainer) return;

  // Group seats by row_name
  const rowsMap = {};
  seats.forEach(s => {
    if (!rowsMap[s.row_name]) {
      rowsMap[s.row_name] = [];
    }
    rowsMap[s.row_name].push(s);
  });

  gridContainer.innerHTML = '';

  for (const rowKey in rowsMap) {
    const rowDiv = document.createElement('div');
    rowDiv.className = 'seat-row';

    // Row Label
    const label = document.createElement('div');
    label.className = 'row-label';
    label.textContent = rowKey;
    rowDiv.appendChild(label);

    // Seats in this row
    rowsMap[rowKey].forEach(seat => {
      const btn = document.createElement('div');
      const isPremium = (seat.category === 'Premium');
      btn.className = `seat-btn ${isPremium ? 'premium' : 'regular'} ${seat.is_booked ? 'booked' : ''}`;
      btn.textContent = seat.seat_number;
      btn.title = `${seat.seat_number} - ${seat.category} (₹${seat.base_price})`;

      if (seat.is_booked) {
        btn.title += ' [ALREADY BOOKED]';
      } else {
        btn.addEventListener('click', () => toggleSeat(seat, btn));
      }

      rowDiv.appendChild(btn);
    });

    gridContainer.appendChild(rowDiv);
  }
}

function toggleSeat(seat, btnElem) {
  if (selectedSeats.has(seat.id)) {
    selectedSeats.delete(seat.id);
    btnElem.classList.remove('selected');
  } else {
    // Max 10 seats per booking
    if (selectedSeats.size >= 10) {
      showToast('You can select a maximum of 10 seats per booking.', 'info');
      return;
    }
    selectedSeats.set(seat.id, seat);
    btnElem.classList.add('selected');
  }

  updateSummary();
}

function updateSummary() {
  const countElem = document.getElementById('selected-count');
  const seatsListElem = document.getElementById('selected-seats-list');
  const totalElem = document.getElementById('total-price');
  const proceedBtn = document.getElementById('proceed-btn');

  let total = 0;
  const seatNames = [];

  selectedSeats.forEach(seat => {
    total += seat.base_price;
    seatNames.push(seat.seat_number);
  });

  if (countElem) countElem.textContent = selectedSeats.size;
  if (seatsListElem) {
    seatsListElem.textContent = seatNames.length > 0 ? seatNames.join(', ') : 'None';
  }
  if (totalElem) totalElem.textContent = formatCurrency(total);

  if (proceedBtn) {
    proceedBtn.disabled = (selectedSeats.size === 0);
  }
}

function proceedToBooking() {
  if (selectedSeats.size === 0) {
    showToast('Please select at least one seat to proceed.', 'error');
    return;
  }

  const user = getCurrentUser();
  if (!user) {
    showToast('Ticket booking requires logging in. Redirecting to login...', 'error');
    setTimeout(() => {
      window.location.href = `login.html?redirect=seats.html?show_id=${currentShowData.show.show_id}&msg=login_required`;
    }, 1200);
    return;
  }

  const seatIds = Array.from(selectedSeats.keys());
  const seatNames = Array.from(selectedSeats.values()).map(s => s.seat_number);
  const showId = currentShowData.show.show_id;

  // Save selected booking session data
  const bookingSession = {
    show_id: showId,
    movie_title: currentShowData.show.movie_title,
    poster_url: currentShowData.show.poster_url,
    theatre_name: currentShowData.show.theatre_name,
    screen_name: currentShowData.show.screen_name,
    show_date: currentShowData.show.show_date,
    show_time: currentShowData.show.show_time,
    seat_ids: seatIds,
    seat_names: seatNames,
    total_amount: Array.from(selectedSeats.values()).reduce((sum, s) => sum + s.base_price, 0)
  };

  sessionStorage.setItem('cinebook_pending_order', JSON.stringify(bookingSession));
  window.location.href = `booking.html?show_id=${showId}`;
}

document.addEventListener('DOMContentLoaded', () => {
  loadShowAndSeats();

  const proceedBtn = document.getElementById('proceed-btn');
  if (proceedBtn) {
    proceedBtn.addEventListener('click', proceedToBooking);
  }
});
