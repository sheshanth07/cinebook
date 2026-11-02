/**
 * CineBook - Admin Dashboard Operations Script
 */

let adminToken = sessionStorage.getItem('cinebook_admin_token') || 'cinebook_admin_secure_key_2026';

function getAdminHeaders() {
  return {
    'Content-Type': 'application/json',
    'X-Admin-Token': adminToken
  };
}

// Tab Switching
function switchTab(tabId) {
  document.querySelectorAll('.admin-tab-content').forEach(el => el.style.display = 'none');
  document.querySelectorAll('.tab-btn').forEach(el => el.classList.remove('active'));

  const activeContent = document.getElementById(`tab-${tabId}`);
  const activeBtn = document.getElementById(`btn-tab-${tabId}`);

  if (activeContent) activeContent.style.display = 'block';
  if (activeBtn) activeBtn.classList.add('active');

  if (tabId === 'overview') loadOverviewStats();
  else if (tabId === 'users') loadAdminUsers();
  else if (tabId === 'movies') loadAdminMovies();
  else if (tabId === 'theatres') loadAdminTheatres();
  else if (tabId === 'shows') loadAdminShows();
  else if (tabId === 'bookings') loadAdminBookings();
}

// ----------------------------------------------------------------------------
// User Accounts Management
// ----------------------------------------------------------------------------
async function loadAdminUsers() {
  const tbody = document.getElementById('admin-users-tbody');
  if (!tbody) return;

  try {
    const res = await fetch(`${API_BASE}/api/admin/users`, { headers: getAdminHeaders() });
    if (!res.ok) throw new Error('Failed to load accounts.');
    const users = await res.json();

    tbody.innerHTML = users.map(u => {
      const isAdmin = (u.role === 'admin');
      const roleBadge = isAdmin 
        ? `<span style="background: rgba(245, 197, 24, 0.15); color: var(--accent-gold); padding: 0.2rem 0.6rem; border-radius: 4px; font-weight: 700; font-size: 0.75rem;">ADMIN</span>`
        : `<span style="background: rgba(56, 189, 248, 0.15); color: #38bdf8; padding: 0.2rem 0.6rem; border-radius: 4px; font-weight: 700; font-size: 0.75rem;">CUSTOMER</span>`;

      return `
        <tr>
          <td><strong>#${u.id}</strong></td>
          <td><strong>${u.username}</strong></td>
          <td>${u.email}</td>
          <td>${roleBadge}</td>
          <td>${u.booking_count} bookings</td>
          <td>${u.created_at || 'N/A'}</td>
          <td>
            ${!isAdmin ? `
              <button class="btn btn-danger btn-sm" onclick="deleteUserAccount(${u.id}, '${u.username}')">
                Delete Account
              </button>
            ` : `<span style="color: var(--text-dim); font-size: 0.8rem; font-style: italic;">Protected Admin</span>`}
          </td>
        </tr>
      `;
    }).join('');
  } catch (err) {
    showToast(err.message, 'error');
  }
}

async function deleteUserAccount(id, name) {
  if (!confirm(`Are you sure you want to permanently delete user account "${name}" (#${id})?\nThis will remove the user and clear/cancel any associated bookings.`)) {
    return;
  }

  try {
    const res = await fetch(`${API_BASE}/api/admin/users/${id}`, {
      method: 'DELETE',
      headers: getAdminHeaders()
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to delete user.');

    showToast(`User account "${name}" has been deleted.`, 'success');
    loadAdminUsers();
    loadOverviewStats();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ----------------------------------------------------------------------------
// Overview Stats
// ----------------------------------------------------------------------------
async function loadOverviewStats() {
  try {
    const res = await fetch(`${API_BASE}/api/admin/stats`, { headers: getAdminHeaders() });
    if (!res.ok) throw new Error('Unauthorized or failed to fetch stats.');
    const s = await res.json();

    document.getElementById('stat-revenue').textContent = formatCurrency(s.total_revenue || 0);
    document.getElementById('stat-bookings').textContent = s.total_bookings || 0;
    document.getElementById('stat-movies').textContent = s.total_movies || 0;
    document.getElementById('stat-shows').textContent = s.total_shows || 0;
    document.getElementById('stat-theatres').textContent = s.total_theatres || 0;
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ----------------------------------------------------------------------------
// Movies Management
// ----------------------------------------------------------------------------
async function loadAdminMovies() {
  const tbody = document.getElementById('admin-movies-tbody');
  if (!tbody) return;

  try {
    const res = await fetch(`${API_BASE}/api/movies`);
    const movies = await res.json();

    tbody.innerHTML = movies.map(m => `
      <tr>
        <td><strong>#${m.id}</strong></td>
        <td>
          <div style="font-weight: 700;">${m.title}</div>
          <div style="font-size: 0.8rem; color: var(--text-muted);">${m.language} • ${m.genre}</div>
        </td>
        <td>★ ${m.rating} (${m.certificate})</td>
        <td>${m.duration_mins} mins</td>
        <td>${m.release_date}</td>
        <td>
          <button class="btn btn-danger btn-sm" onclick="deleteMovie(${m.id})">Delete</button>
        </td>
      </tr>
    `).join('');
  } catch (err) {
    showToast('Failed to load movies table', 'error');
  }
}

async function handleAddMovie(e) {
  e.preventDefault();

  const title = document.getElementById('movie-title').value.trim();
  const genre = document.getElementById('movie-genre').value;
  const language = document.getElementById('movie-lang').value;
  const duration = parseInt(document.getElementById('movie-duration').value) || 120;
  const releaseDate = document.getElementById('movie-release').value;
  const certificate = document.getElementById('movie-cert').value;
  const rating = parseFloat(document.getElementById('movie-rating').value) || 7.0;
  const director = document.getElementById('movie-director').value.trim();
  const cast = document.getElementById('movie-cast').value.trim();
  const poster = document.getElementById('movie-poster').value.trim();
  const description = document.getElementById('movie-desc').value.trim();

  try {
    const res = await fetch(`${API_BASE}/api/admin/movies`, {
      method: 'POST',
      headers: getAdminHeaders(),
      body: JSON.stringify({
        title, genre, language, duration_mins: duration, release_date: releaseDate,
        certificate, rating, director, cast_details: cast, poster_url: poster,
        description, is_upcoming: 0
      })
    });

    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to add movie.');

    showToast(`Movie "${title}" saved to database!`, 'success');
    document.getElementById('add-movie-form').reset();
    closeModal('modal-add-movie');
    loadAdminMovies();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

async function deleteMovie(id) {
  if (!confirm(`Are you sure you want to delete movie #${id}?\nSafety constraint: deletion will fail if active scheduled shows reference this movie.`)) {
    return;
  }

  try {
    const res = await fetch(`${API_BASE}/api/admin/movies/${id}`, {
      method: 'DELETE',
      headers: getAdminHeaders()
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to delete movie.');

    showToast('Movie deleted successfully.', 'success');
    loadAdminMovies();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ----------------------------------------------------------------------------
// Theatres & Screens Management
// ----------------------------------------------------------------------------
async function loadAdminTheatres() {
  const tbody = document.getElementById('admin-theatres-tbody');
  const screensTbody = document.getElementById('admin-screens-tbody');
  const theatreSelect = document.getElementById('screen-theatre-select');

  try {
    const [tRes, sRes] = await Promise.all([
      fetch(`${API_BASE}/api/admin/theatres`, { headers: getAdminHeaders() }),
      fetch(`${API_BASE}/api/admin/screens`, { headers: getAdminHeaders() })
    ]);

    const theatres = await tRes.json();
    const screens = await sRes.json();

    if (tbody) {
      tbody.innerHTML = theatres.map(t => `
        <tr>
          <td>#${t.id}</td>
          <td><strong>${t.name}</strong></td>
          <td>${t.location}</td>
          <td>${t.city}</td>
        </tr>
      `).join('');
    }

    if (screensTbody) {
      screensTbody.innerHTML = screens.map(sc => `
        <tr>
          <td>#${sc.id}</td>
          <td><strong>${sc.screen_name}</strong></td>
          <td>${sc.theatre_name} (${sc.city})</td>
          <td>${sc.total_seats} seats</td>
        </tr>
      `).join('');
    }

    if (theatreSelect) {
      theatreSelect.innerHTML = theatres.map(t => `
        <option value="${t.id}">${t.name} (${t.city})</option>
      `).join('');
    }
  } catch (err) {
    showToast('Failed to load theatres & screens', 'error');
  }
}

async function handleAddTheatre(e) {
  e.preventDefault();
  const name = document.getElementById('theatre-name').value.trim();
  const location = document.getElementById('theatre-location').value.trim();
  const city = document.getElementById('theatre-city').value.trim();

  try {
    const res = await fetch(`${API_BASE}/api/admin/theatres`, {
      method: 'POST',
      headers: getAdminHeaders(),
      body: JSON.stringify({ name, location, city })
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to add theatre.');

    showToast(`Theatre "${name}" added to database!`, 'success');
    document.getElementById('add-theatre-form').reset();
    closeModal('modal-add-theatre');
    loadAdminTheatres();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

async function handleAddScreen(e) {
  e.preventDefault();
  const theatreId = parseInt(document.getElementById('screen-theatre-select').value);
  const screenName = document.getElementById('screen-name').value.trim();
  const totalSeats = parseInt(document.getElementById('screen-seats').value) || 25;

  try {
    const res = await fetch(`${API_BASE}/api/admin/screens`, {
      method: 'POST',
      headers: getAdminHeaders(),
      body: JSON.stringify({ theatre_id: theatreId, screen_name: screenName, total_seats: totalSeats })
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to create screen.');

    showToast(`Screen "${screenName}" created with 25 auto-generated seats!`, 'success');
    document.getElementById('add-screen-form').reset();
    closeModal('modal-add-screen');
    loadAdminTheatres();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ----------------------------------------------------------------------------
// Shows Management
// ----------------------------------------------------------------------------
async function loadAdminShows() {
  const tbody = document.getElementById('admin-shows-tbody');
  const movieSelect = document.getElementById('show-movie-select');
  const screenSelect = document.getElementById('show-screen-select');

  try {
    const [showsRes, moviesRes, screensRes] = await Promise.all([
      fetch(`${API_BASE}/api/admin/shows`, { headers: getAdminHeaders() }),
      fetch(`${API_BASE}/api/movies`),
      fetch(`${API_BASE}/api/admin/screens`, { headers: getAdminHeaders() })
    ]);

    const shows = await showsRes.json();
    const movies = await moviesRes.json();
    const screens = await screensRes.json();

    if (tbody) {
      tbody.innerHTML = shows.map(s => `
        <tr>
          <td>#${s.id}</td>
          <td><strong>${s.movie_title}</strong></td>
          <td>${s.theatre_name} - ${s.screen_name}</td>
          <td>📅 ${s.show_date} • ⏰ ${s.show_time}</td>
          <td>
            <span style="color: ${s.booked_count > 0 ? '#f5c518' : 'var(--text-muted)'}; font-weight: 600;">
              ${s.booked_count} / ${s.total_seats} booked
            </span>
          </td>
          <td>
            <button class="btn btn-danger btn-sm" onclick="deleteShow(${s.id})">Delete</button>
          </td>
        </tr>
      `).join('');
    }

    if (movieSelect) {
      movieSelect.innerHTML = movies.map(m => `<option value="${m.id}">${m.title}</option>`).join('');
    }

    if (screenSelect) {
      screenSelect.innerHTML = screens.map(sc => `<option value="${sc.id}">${sc.theatre_name} - ${sc.screen_name}</option>`).join('');
    }
  } catch (err) {
    showToast('Failed to load shows table', 'error');
  }
}

async function handleAddShow(e) {
  e.preventDefault();
  const movieId = parseInt(document.getElementById('show-movie-select').value);
  const screenId = parseInt(document.getElementById('show-screen-select').value);
  const showDate = document.getElementById('show-date-input').value;
  const showTime = document.getElementById('show-time-input').value;

  try {
    const res = await fetch(`${API_BASE}/api/admin/shows`, {
      method: 'POST',
      headers: getAdminHeaders(),
      body: JSON.stringify({
        movie_id: movieId,
        screen_id: screenId,
        show_date: showDate,
        show_time: showTime
      })
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to schedule show.');

    showToast(`Show scheduled successfully for ${showDate} at ${showTime}!`, 'success');
    document.getElementById('add-show-form').reset();
    closeModal('modal-add-show');
    loadAdminShows();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

async function deleteShow(id) {
  if (!confirm(`Are you sure you want to cancel and remove show #${id}?\nThis will cancel the show and release/clear any active seat bookings.`)) {
    return;
  }

  try {
    const res = await fetch(`${API_BASE}/api/admin/shows/${id}`, {
      method: 'DELETE',
      headers: getAdminHeaders()
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to delete show.');

    showToast('Show deleted successfully.', 'success');
    loadAdminShows();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

// ----------------------------------------------------------------------------
// Bookings Management
// ----------------------------------------------------------------------------
async function loadAdminBookings() {
  const tbody = document.getElementById('admin-bookings-tbody');
  if (!tbody) return;

  try {
    const res = await fetch(`${API_BASE}/api/admin/bookings`, { headers: getAdminHeaders() });
    const bookings = await res.json();

    tbody.innerHTML = bookings.map(b => {
      const isCancelled = (b.booking_status === 'CANCELLED');
      const statusColor = isCancelled ? '#ef4444' : '#22c55e';

      return `
        <tr>
          <td><strong style="color: var(--accent-gold);">${b.booking_reference}</strong></td>
          <td>
            <div>${b.customer_name}</div>
            <div style="font-size: 0.8rem; color: var(--text-muted);">${b.customer_email}</div>
          </td>
          <td>${b.movie_title}</td>
          <td>${b.theatre_name} (${b.screen_name})</td>
          <td>📅 ${b.show_date} ${b.show_time}</td>
          <td><strong>${formatCurrency(b.total_amount)}</strong> (${b.ticket_count} tickets)</td>
          <td><span style="color: ${statusColor}; font-weight: 700;">${b.booking_status}</span></td>
          <td>
            ${!isCancelled ? `
              <button class="btn btn-danger btn-sm" onclick="adminCancelBooking('${b.booking_reference}')">
                Cancel & Refund
              </button>
            ` : `<span style="font-size: 0.8rem; color: var(--text-muted);">Refunded</span>`}
          </td>
        </tr>
      `;
    }).join('');
  } catch (err) {
    showToast('Failed to load bookings list', 'error');
  }
}

async function adminCancelBooking(ref) {
  if (!confirm(`Cancel booking ${ref} and release seats?`)) return;

  try {
    const res = await fetch(`${API_BASE}/api/admin/bookings/${ref}/status`, {
      method: 'PUT',
      headers: getAdminHeaders(),
      body: JSON.stringify({ status: 'CANCELLED' })
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || 'Failed to update status.');

    showToast('Booking cancelled and seats released.', 'success');
    loadAdminBookings();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

document.addEventListener('DOMContentLoaded', () => {
  switchTab('overview');

  // Form submit listeners
  const addMovieForm = document.getElementById('add-movie-form');
  if (addMovieForm) addMovieForm.addEventListener('submit', handleAddMovie);

  const addTheatreForm = document.getElementById('add-theatre-form');
  if (addTheatreForm) addTheatreForm.addEventListener('submit', handleAddTheatre);

  const addScreenForm = document.getElementById('add-screen-form');
  if (addScreenForm) addScreenForm.addEventListener('submit', handleAddScreen);

  const addShowForm = document.getElementById('add-show-form');
  if (addShowForm) addShowForm.addEventListener('submit', handleAddShow);
});
