/**
 * CineBook - Movie Ticket Booking System
 * Shared Common Utilities, Navigation, and Authentication Protection
 */

const API_BASE = (window.location.protocol === 'file:' || !window.location.port) 
  ? 'http://localhost:8080' 
  : '';

// Authentication Helpers
function getCurrentUser() {
  try {
    const raw = localStorage.getItem('cinebook_user');
    return raw ? JSON.parse(raw) : null;
  } catch (e) {
    return null;
  }
}

function logout() {
  localStorage.removeItem('cinebook_user');
  sessionStorage.removeItem('cinebook_admin_token');
  showToast('You have been logged out.', 'info');
  setTimeout(() => {
    window.location.href = 'login.html';
  }, 500);
}

// Enforce Auth Gates on Page Load
function checkRouteProtection() {
  const path = window.location.pathname.toLowerCase();
  const user = getCurrentUser();

  // Root or index.html must start on login screen if not logged in (unless guest browsing)
  if ((path.endsWith('/') || path.endsWith('/index.html') || path === '') && !user) {
    if (sessionStorage.getItem('cinebook_guest_browse') !== '1') {
      window.location.href = 'login.html';
      return;
    }
  }

  // Admin page requires admin role
  if (path.includes('admin.html')) {
    if (!user || user.role !== 'admin') {
      window.location.href = 'login.html?mode=admin&msg=admin_required';
      return;
    }
  }

  // Booking page requires logged-in user
  if (path.includes('booking.html')) {
    if (!user) {
      window.location.href = 'login.html?redirect=booking.html&msg=login_required';
      return;
    }
  }
}

// Toast Notification System
function showToast(message, type = 'info') {
  let container = document.getElementById('toast-container');
  if (!container) {
    container = document.createElement('div');
    container.id = 'toast-container';
    document.body.appendChild(container);
  }

  const toast = document.createElement('div');
  toast.className = `toast toast-${type}`;
  toast.innerHTML = `<span>${message}</span>`;

  container.appendChild(toast);

  setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateX(100%)';
    toast.style.transition = 'all 0.3s ease';
    setTimeout(() => toast.remove(), 300);
  }, 4000);
}

// Currency Formatter
function formatCurrency(amount) {
  return '₹' + Number(amount).toFixed(2);
}

// DB Status Badge removed as per user preference
async function updateDbStatusBadge() {
  const badge = document.getElementById('db-status-badge');
  if (badge) badge.style.display = 'none';
}

// Dynamic Navigation Bar User Info & Logout
function renderNavUserSection() {
  const navLinks = document.querySelector('.nav-links');
  if (!navLinks) return;

  const user = getCurrentUser();
  const existingUserItem = document.getElementById('nav-user-item');
  if (existingUserItem) existingUserItem.remove();

  const userLi = document.createElement('li');
  userLi.id = 'nav-user-item';

  if (user) {
    const isAdm = (user.role === 'admin');
    userLi.innerHTML = `
      <div style="display: flex; align-items: center; gap: 0.65rem; background: rgba(255,255,255,0.06); padding: 0.35rem 0.75rem; border-radius: var(--radius-sm); border: 1px solid var(--border-color);">
        <span style="font-size: 0.85rem; font-weight: 600; color: ${isAdm ? 'var(--accent-gold)' : '#38bdf8'};">
          ${isAdm ? '🔐 Admin:' : '👤'} ${user.username}
        </span>
        <button onclick="logout()" style="background: none; border: none; color: #ef4444; font-size: 0.8rem; font-weight: 600; cursor: pointer; padding: 0.1rem 0.35rem; border-radius: 3px;">
          (Logout)
        </button>
      </div>
    `;
  } else {
    userLi.innerHTML = `
      <a href="login.html" class="btn btn-primary btn-sm" style="padding: 0.35rem 0.75rem; font-size: 0.85rem;">
        Login / Register
      </a>
    `;
  }

  navLinks.appendChild(userLi);
}

// Modal Helpers
function openModal(id) {
  const modal = document.getElementById(id);
  if (modal) modal.classList.add('active');
}

function closeModal(id) {
  const modal = document.getElementById(id);
  if (modal) modal.classList.remove('active');
}

// Close modals when clicking backdrop
document.addEventListener('click', (e) => {
  if (e.target.classList.contains('modal-overlay')) {
    e.target.classList.remove('active');
  }
});

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
  checkRouteProtection();
  renderNavUserSection();
  updateDbStatusBadge();
});
