/**
 * CineBook - Movies Catalog and Filter Script
 */

let allMovies = [];

// Fallback poster image in case external URL is unreachable
const FALLBACK_POSTER = 'https://images.unsplash.com/photo-1489599849927-2ee91cede3ba?w=600';

async function loadGenresAndLanguages() {
  try {
    const [genresRes, langsRes] = await Promise.all([
      fetch(`${API_BASE}/api/genres`),
      fetch(`${API_BASE}/api/languages`)
    ]);

    if (genresRes.ok) {
      const genres = await genresRes.json();
      const genreSelect = document.getElementById('genre-filter');
      if (genreSelect) {
        genres.forEach(g => {
          const opt = document.createElement('option');
          opt.value = g;
          opt.textContent = g;
          genreSelect.appendChild(opt);
        });
      }
    }

    if (langsRes.ok) {
      const langs = await langsRes.json();
      const langSelect = document.getElementById('lang-filter');
      if (langSelect) {
        langs.forEach(l => {
          const opt = document.createElement('option');
          opt.value = l;
          opt.textContent = l;
          langSelect.appendChild(opt);
        });
      }
    }
  } catch (err) {
    console.error('Error loading filters:', err);
  }
}

async function fetchMovies() {
  const search = document.getElementById('search-input')?.value || '';
  const genre = document.getElementById('genre-filter')?.value || '';
  const language = document.getElementById('lang-filter')?.value || '';

  const params = new URLSearchParams();
  if (search) params.append('search', search);
  if (genre) params.append('genre', genre);
  if (language) params.append('language', language);

  const container = document.getElementById('movies-container');
  if (container) {
    container.innerHTML = `<div style="grid-column: 1/-1; text-align: center; padding: 3rem; color: var(--text-muted);">
      Loading movies from database...
    </div>`;
  }

  try {
    const res = await fetch(`${API_BASE}/api/movies?${params.toString()}`);
    if (!res.ok) throw new Error('Failed to load movies from backend');
    const movies = await res.json();
    renderMovies(movies);
  } catch (err) {
    console.error(err);
    if (container) {
      container.innerHTML = `<div style="grid-column: 1/-1; text-align: center; padding: 3rem; color: #ef4444;">
        Failed to connect to backend database. Please ensure cinebook_server is running.
      </div>`;
    }
  }
}

function renderMovies(movies) {
  const container = document.getElementById('movies-container');
  if (!container) return;

  if (!Array.isArray(movies) || movies.length === 0) {
    container.innerHTML = `
      <div style="grid-column: 1/-1; text-align: center; padding: 4rem 1rem; color: var(--text-muted);">
        <p style="font-size: 1.25rem; font-weight: 600; margin-bottom: 0.5rem;">No movies found</p>
        <p style="font-size: 0.9rem;">Try adjusting your search criteria or clearing filters.</p>
      </div>
    `;
    return;
  }

  container.innerHTML = movies.map(m => {
    const ratingVal = Number(m.rating || 0).toFixed(1);
    const posterSrc = m.poster_url || FALLBACK_POSTER;
    return `
    <div class="movie-card">
      <div class="poster-wrapper">
        <img class="poster-img" src="${posterSrc}" alt="${m.title}" 
             onerror="this.onerror=null; this.src='${FALLBACK_POSTER}';" />
        <span class="rating-badge">★ ${ratingVal}</span>
        <span class="cert-badge">${m.certificate || 'UA'}</span>
      </div>
      <div class="movie-info">
        <h3 class="movie-title">${m.title}</h3>
        <div class="movie-meta">
          <span>${m.language}</span>
          <span class="meta-dot">•</span>
          <span class="genre-tag">${m.genre}</span>
          <span class="meta-dot">•</span>
          <span>${m.duration_mins}m</span>
        </div>
        <p style="font-size: 0.85rem; color: var(--text-dim); margin-bottom: 1.25rem; display: -webkit-box; -webkit-line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden;">
          ${m.description || ''}
        </p>
        <div class="movie-actions">
          <button class="btn btn-secondary btn-sm" style="flex:1" onclick="showMovieDetails(${m.id})">Details</button>
          <a href="shows.html?movie_id=${m.id}" class="btn btn-primary btn-sm" style="flex:1">Book Tickets</a>
        </div>
      </div>
    </div>
  `}).join('');
}

async function showMovieDetails(id) {
  try {
    const res = await fetch(`${API_BASE}/api/movies/${id}`);
    if (!res.ok) throw new Error('Movie details not found');
    const m = await res.json();

    const modalBody = document.getElementById('movie-modal-body');
    if (!modalBody) return;

    modalBody.innerHTML = `
      <div style="display: flex; gap: 1.5rem; flex-wrap: wrap;">
        <img src="${m.poster_url || FALLBACK_POSTER}" alt="${m.title}" 
             style="width: 140px; height: 210px; object-fit: cover; border-radius: var(--radius-sm);" 
             onerror="this.onerror=null; this.src='${FALLBACK_POSTER}';" />
        <div style="flex: 1; min-width: 250px;">
          <h2 style="font-size: 1.6rem; margin-bottom: 0.4rem;">${m.title}</h2>
          <div style="display: flex; gap: 0.6rem; align-items: center; margin-bottom: 0.75rem;">
            <span class="rating-badge" style="position: static;">★ ${m.rating.toFixed(1)}/10</span>
            <span class="cert-badge" style="position: static;">${m.certificate}</span>
            <span style="color: var(--text-muted); font-size: 0.9rem;">${m.duration_mins} mins</span>
          </div>
          <p style="color: var(--accent-cyan); font-weight: 600; font-size: 0.9rem; margin-bottom: 0.5rem;">
            ${m.language} • ${m.genre}
          </p>
          <p style="color: var(--text-muted); font-size: 0.9rem; margin-bottom: 0.5rem;">
            <strong>Director:</strong> ${m.director || 'N/A'}
          </p>
          <p style="color: var(--text-muted); font-size: 0.9rem;">
            <strong>Cast:</strong> ${m.cast_details || 'N/A'}
          </p>
        </div>
      </div>
      <div style="margin-top: 1.5rem; border-top: 1px solid var(--border-color); padding-top: 1rem;">
        <h4 style="margin-bottom: 0.5rem; color: var(--text-main);">Synopsis</h4>
        <p style="color: #cbd5e1; font-size: 0.95rem; line-height: 1.6;">${m.description}</p>
      </div>
      <div style="margin-top: 2rem; display: flex; justify-content: flex-end; gap: 1rem;">
        <button class="btn btn-secondary" onclick="closeModal('movie-modal')">Close</button>
        <a href="shows.html?movie_id=${m.id}" class="btn btn-primary">Check Showtimes & Book</a>
      </div>
    `;

    openModal('movie-modal');
  } catch (err) {
    showToast(err.message, 'error');
  }
}

document.addEventListener('DOMContentLoaded', () => {
  loadGenresAndLanguages();
  fetchMovies();

  // Attach search listeners
  const searchInput = document.getElementById('search-input');
  if (searchInput) {
    let debounceTimer;
    searchInput.addEventListener('input', () => {
      clearTimeout(debounceTimer);
      debounceTimer = setTimeout(fetchMovies, 300);
    });
  }

  const genreFilter = document.getElementById('genre-filter');
  if (genreFilter) genreFilter.addEventListener('change', fetchMovies);

  const langFilter = document.getElementById('lang-filter');
  if (langFilter) langFilter.addEventListener('change', fetchMovies);
});
