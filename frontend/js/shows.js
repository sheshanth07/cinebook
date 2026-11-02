/**
 * CineBook - Showtimes & Theatre Schedule Script
 * Clean, humanized, and student-friendly implementation
 */

let selectedDate = '';
let currentMovieId = 0;

function formatShowDate(dateStr) {
  if (!dateStr) return '';
  try {
    const parts = dateStr.split('-');
    if (parts.length !== 3) return dateStr;
    const y = parseInt(parts[0]);
    const m = parseInt(parts[1]) - 1;
    const d = parseInt(parts[2]);
    const dt = new Date(y, m, d);
    const days = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
    const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
    return `${days[dt.getDay()]}, ${months[dt.getMonth()]} ${dt.getDate()}`;
  } catch (e) {
    return dateStr;
  }
}

function setupDateSelector() {
  const container = document.getElementById('date-selector');
  if (!container) return;

  container.innerHTML = '';

  // 1. "All Dates" Tab
  const allBtn = document.createElement('div');
  allBtn.id = 'date-btn-all';
  allBtn.className = `date-btn ${selectedDate === '' ? 'active' : ''}`;
  allBtn.innerHTML = `
    <div class="day-name">ALL</div>
    <div class="day-num">DATES</div>
  `;
  allBtn.addEventListener('click', () => {
    document.querySelectorAll('.date-btn').forEach(b => b.classList.remove('active'));
    allBtn.classList.add('active');
    selectedDate = '';
    fetchShows();
  });
  container.appendChild(allBtn);

  // 2. Next 5 Days Tabs
  const days = ['SUN', 'MON', 'TUE', 'WED', 'THU', 'FRI', 'SAT'];

  for (let i = 0; i < 5; i++) {
    const d = new Date();
    d.setDate(d.getDate() + i);

    const year = d.getFullYear();
    const month = String(d.getMonth() + 1).padStart(2, '0');
    const day = String(d.getDate()).padStart(2, '0');
    const dateStr = `${year}-${month}-${day}`;

    const btn = document.createElement('div');
    btn.className = `date-btn ${selectedDate === dateStr ? 'active' : ''}`;
    btn.innerHTML = `
      <div class="day-name">${i === 0 ? 'TODAY' : days[d.getDay()]}</div>
      <div class="day-num">${d.getDate()}</div>
    `;

    btn.addEventListener('click', () => {
      document.querySelectorAll('.date-btn').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      selectedDate = dateStr;
      fetchShows();
    });

    container.appendChild(btn);
  }
}

async function loadMovieSelector() {
  const select = document.getElementById('movie-select');
  if (!select) return;

  try {
    const res = await fetch(`${API_BASE}/api/movies`);
    if (!res.ok) return;
    const movies = await res.json();

    select.innerHTML = '<option value="0">All Movies</option>';
    if (Array.isArray(movies)) {
      movies.forEach(m => {
        const opt = document.createElement('option');
        opt.value = m.id;
        opt.textContent = `${m.title} (${m.language})`;
        if (m.id === currentMovieId) opt.selected = true;
        select.appendChild(opt);
      });
    }

    select.addEventListener('change', (e) => {
      currentMovieId = parseInt(e.target.value) || 0;
      fetchShows();
    });
  } catch (err) {
    console.error('Error loading movies in showtimes:', err);
  }
}

async function fetchShows() {
  const container = document.getElementById('shows-container');
  if (!container) return;

  container.innerHTML = `<div style="text-align: center; padding: 3rem; color: var(--text-muted);">
    Loading scheduled showtimes from database...
  </div>`;

  try {
    const params = new URLSearchParams();
    if (currentMovieId > 0) params.append('movie_id', currentMovieId);
    if (selectedDate) params.append('date', selectedDate);

    const res = await fetch(`${API_BASE}/api/shows?${params.toString()}`);
    if (!res.ok) throw new Error('Failed to fetch shows from backend');
    const shows = await res.json();
    renderShows(shows);
  } catch (err) {
    console.error(err);
    container.innerHTML = `<div style="text-align: center; padding: 3rem; color: #ef4444;">
      Could not load shows. Please ensure cinebook_server is running on port 8080.
    </div>`;
  }
}

function resetFilters() {
  currentMovieId = 0;
  selectedDate = '';
  const movieSelect = document.getElementById('movie-select');
  if (movieSelect) movieSelect.value = '0';
  document.querySelectorAll('.date-btn').forEach(b => b.classList.remove('active'));
  const allBtn = document.getElementById('date-btn-all');
  if (allBtn) allBtn.classList.add('active');
  fetchShows();
}

function renderShows(shows) {
  const container = document.getElementById('shows-container');
  if (!container) return;

  if (!Array.isArray(shows) || shows.length === 0) {
    container.innerHTML = `
      <div class="card" style="text-align: center; padding: 3rem 1rem;">
        <h3 style="margin-bottom: 0.5rem; color: var(--text-main);">No Showtimes Found</h3>
        <p style="color: var(--text-muted); font-size: 0.95rem; margin-bottom: 1.5rem;">
          There are no shows matching your current filter in the database.
        </p>
        <button class="btn btn-secondary btn-sm" onclick="resetFilters()">
          View All Available Shows
        </button>
      </div>
    `;
    return;
  }

  // Group shows by Theatre
  const theatreMap = {};
  shows.forEach(s => {
    if (!theatreMap[s.theatre_id]) {
      theatreMap[s.theatre_id] = {
        name: s.theatre_name,
        city: s.city,
        shows: []
      };
    }
    theatreMap[s.theatre_id].shows.push(s);
  });

  let html = '';
  for (const tId in theatreMap) {
    const t = theatreMap[tId];
    html += `
      <div class="theatre-show-card">
        <div class="theatre-header">
          <div>
            <div class="theatre-name">🏛️ ${t.name}</div>
            <div class="theatre-loc">📍 ${t.city}</div>
          </div>
          <span style="font-size: 0.85rem; color: var(--accent-gold); font-weight: 600;">
            ✓ M-Ticket & Online Booking Available
          </span>
        </div>
        <div class="showtimes-list">
    `;

    t.shows.forEach(s => {
      const seatsLeft = s.available_seats;
      const seatColor = seatsLeft < 5 ? '#ef4444' : '#22c55e';
      const formattedDate = formatShowDate(s.show_date);

      html += `
        <a href="seats.html?show_id=${s.show_id}" class="showtime-chip" title="Click to select seats for ${s.movie_title}">
          <span style="font-size: 0.72rem; background: rgba(255,255,255,0.08); padding: 0.15rem 0.5rem; border-radius: 4px; color: var(--accent-cyan); font-weight: 600; margin-bottom: 0.35rem;">
            📅 ${formattedDate}
          </span>
          <span class="showtime-time">⏰ ${s.show_time}</span>
          <span class="showtime-screen" style="margin-top: 0.2rem;">${s.screen_name}</span>
          <span style="font-size: 0.85rem; color: var(--text-main); font-weight: 700; margin-top: 0.3rem;">
            ${s.movie_title}
          </span>
          <span style="font-size: 0.75rem; color: var(--text-dim); margin-top: 0.1rem;">
            ${s.certificate} • ${s.duration_mins} mins
          </span>
          <span class="showtime-seats" style="color: ${seatColor}; margin-top: 0.4rem; font-weight: 700;">
            ${seatsLeft} seats left
          </span>
          <span class="btn btn-primary btn-sm" style="margin-top: 0.5rem; width: 100%; text-align: center; padding: 0.25rem 0.5rem; font-size: 0.75rem;">
            Select Seats →
          </span>
        </a>
      `;
    });

    html += `
        </div>
      </div>
    `;
  }

  container.innerHTML = html;
}

document.addEventListener('DOMContentLoaded', () => {
  const urlParams = new URLSearchParams(window.location.search);
  const mId = urlParams.get('movie_id');
  if (mId) {
    currentMovieId = parseInt(mId);
  }

  // Default to All Dates so shows are never hidden
  selectedDate = '';

  setupDateSelector();
  loadMovieSelector();
  fetchShows();
});
