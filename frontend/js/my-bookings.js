/**
 * CineBook - My Bookings Lookup & Cancellation Script
 */

async function searchBookings(e) {
  if (e) e.preventDefault();

  const refInput = document.getElementById('search-ref');
  const emailInput = document.getElementById('search-email');
  const resultsContainer = document.getElementById('bookings-results');

  const ref = refInput ? refInput.value.trim() : '';
  const email = emailInput ? emailInput.value.trim() : '';

  if (!ref && !email) {
    showToast('Please enter either a Booking Reference or an Email address.', 'error');
    return;
  }

  resultsContainer.innerHTML = `<div style="text-align: center; padding: 3rem; color: var(--text-muted);">
    Searching database records...
  </div>`;

  try {
    if (ref) {
      // Lookup specific booking by reference
      const url = `${API_BASE}/api/bookings/${encodeURIComponent(ref)}${email ? `?email=${encodeURIComponent(email)}` : ''}`;
      const res = await fetch(url);
      if (!res.ok) {
        const err = await res.json();
        throw new Error(err.error || 'Booking reference not found.');
      }
      const booking = await res.json();
      renderSingleBooking(booking, resultsContainer);
    } else {
      // Lookup all bookings by user email
      const res = await fetch(`${API_BASE}/api/bookings?email=${encodeURIComponent(email)}`);
      if (!res.ok) {
        const err = await res.json();
        throw new Error(err.error || 'Failed to retrieve bookings.');
      }
      const bookings = await res.json();
      renderBookingsList(bookings, resultsContainer);
    }
  } catch (err) {
    resultsContainer.innerHTML = `
      <div class="card" style="text-align: center; padding: 2.5rem; border-color: rgba(239, 68, 68, 0.4);">
        <p style="color: #ef4444; font-weight: 600; margin-bottom: 0.5rem;">No Records Found</p>
        <p style="color: var(--text-muted); font-size: 0.9rem;">${err.message}</p>
      </div>
    `;
  }
}

function renderSingleBooking(b, container) {
  const isCancelled = (b.booking_status === 'CANCELLED');
  const statusColor = isCancelled ? '#ef4444' : '#22c55e';
  const statusBg = isCancelled ? 'rgba(239, 68, 68, 0.15)' : 'rgba(34, 197, 94, 0.15)';

  container.innerHTML = `
    <div class="card" style="border-left: 4px solid ${statusColor};">
      <div style="display: flex; justify-content: space-between; align-items: flex-start; flex-wrap: wrap; gap: 1rem; margin-bottom: 1.25rem;">
        <div>
          <span style="font-size: 0.75rem; text-transform: uppercase; color: var(--text-dim); letter-spacing: 0.5px;">Booking Reference</span>
          <h3 style="font-size: 1.35rem; color: var(--accent-gold);">${b.booking_reference}</h3>
        </div>
        <span style="background: ${statusBg}; color: ${statusColor}; border: 1px solid ${statusColor}; padding: 0.35rem 0.85rem; border-radius: 999px; font-size: 0.85rem; font-weight: 700;">
          ${b.booking_status}
        </span>
      </div>

      <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 1rem; margin-bottom: 1.5rem; font-size: 0.9rem;">
        <div>
          <span style="color: var(--text-muted); display: block; font-size: 0.8rem;">Movie Title</span>
          <strong>${b.movie_title}</strong>
        </div>
        <div>
          <span style="color: var(--text-muted); display: block; font-size: 0.8rem;">Cinema & Screen</span>
          <strong>${b.theatre_name} (${b.screen_name})</strong>
        </div>
        <div>
          <span style="color: var(--text-muted); display: block; font-size: 0.8rem;">Date & Time</span>
          <strong>${b.show_date} • ${b.show_time}</strong>
        </div>
        <div>
          <span style="color: var(--text-muted); display: block; font-size: 0.8rem;">Customer</span>
          <strong>${b.customer_name} (${b.customer_email})</strong>
        </div>
      </div>

      <div style="background: var(--bg-card); padding: 1rem; border-radius: var(--radius-sm); margin-bottom: 1.5rem; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 1rem;">
        <div>
          <span style="color: var(--text-muted); font-size: 0.85rem;">Reserved Seats (${b.ticket_count}):</span>
          <div style="font-weight: 700; color: #4ade80; font-size: 1.1rem;">
            ${b.seats.map(s => s.seat_number).join(', ')}
          </div>
        </div>
        <div>
          <span style="color: var(--text-muted); font-size: 0.85rem;">Total Paid:</span>
          <div style="font-weight: 800; color: #22c55e; font-size: 1.2rem;">
            ${formatCurrency(b.total_amount)}
          </div>
        </div>
      </div>

      <div style="display: flex; justify-content: flex-end; gap: 1rem;">
        ${!isCancelled ? `
          <button class="btn btn-danger btn-sm" onclick="cancelBookingPrompt('${b.booking_reference}')">
            Cancel Booking & Release Seats
          </button>
        ` : `
          <span style="color: var(--text-muted); font-size: 0.85rem; font-style: italic;">
            Booking was cancelled. Payment refunded.
          </span>
        `}
      </div>
    </div>
  `;
}

function renderBookingsList(bookings, container) {
  if (bookings.length === 0) {
    container.innerHTML = `
      <div class="card" style="text-align: center; padding: 3rem;">
        <p style="color: var(--text-muted);">No bookings associated with this email address.</p>
      </div>
    `;
    return;
  }

  container.innerHTML = bookings.map(b => {
    const isCancelled = (b.booking_status === 'CANCELLED');
    const statusColor = isCancelled ? '#ef4444' : '#22c55e';
    const statusBg = isCancelled ? 'rgba(239, 68, 68, 0.15)' : 'rgba(34, 197, 94, 0.15)';

    return `
      <div class="card" style="border-left: 4px solid ${statusColor}; margin-bottom: 1.25rem;">
        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 1rem; flex-wrap: wrap; gap: 0.5rem;">
          <div>
            <span style="font-size: 0.75rem; text-transform: uppercase; color: var(--text-dim);">Reference</span>
            <div style="font-weight: 800; color: var(--accent-gold);">${b.booking_reference}</div>
          </div>
          <span style="background: ${statusBg}; color: ${statusColor}; padding: 0.25rem 0.75rem; border-radius: 999px; font-size: 0.8rem; font-weight: 700;">
            ${b.booking_status}
          </span>
        </div>

        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 0.75rem; font-size: 0.9rem; margin-bottom: 1rem;">
          <div><strong>${b.movie_title}</strong></div>
          <div>${b.theatre_name} (${b.screen_name})</div>
          <div>📅 ${b.show_date} • ⏰ ${b.show_time}</div>
          <div>Seats: <span style="color: #4ade80; font-weight: 700;">${b.seats_summary}</span></div>
          <div>Total: <span style="color: #22c55e; font-weight: 700;">${formatCurrency(b.total_amount)}</span></div>
        </div>

        <div style="display: flex; justify-content: flex-end;">
          ${!isCancelled ? `
            <button class="btn btn-danger btn-sm" onclick="cancelBookingPrompt('${b.booking_reference}')">
              Cancel Booking
            </button>
          ` : `
            <span style="color: var(--text-muted); font-size: 0.8rem; font-style: italic;">Cancelled & Refunded</span>
          `}
        </div>
      </div>
    `;
  }).join('');
}

async function cancelBookingPrompt(ref) {
  if (!confirm(`Are you sure you want to cancel booking ${ref}?\nThis will permanently update the database and release all reserved seats back into availability.`)) {
    return;
  }

  try {
    const res = await fetch(`${API_BASE}/api/bookings/${ref}/cancel`, {
      method: 'POST'
    });
    const data = await res.json();

    if (!res.ok) {
      throw new Error(data.error || 'Failed to cancel booking.');
    }

    showToast(data.message || 'Booking cancelled successfully.', 'success');
    // Refresh search results
    searchBookings();
  } catch (err) {
    showToast(err.message, 'error');
  }
}

document.addEventListener('DOMContentLoaded', () => {
  const form = document.getElementById('search-booking-form');
  if (form) form.addEventListener('submit', searchBookings);

  // Check URL parameters (e.g. redirected from booking confirmation)
  const urlParams = new URLSearchParams(window.location.search);
  const ref = urlParams.get('ref');
  const email = urlParams.get('email');

  if (ref) {
    const refInput = document.getElementById('search-ref');
    if (refInput) refInput.value = ref;
  }
  if (email) {
    const emailInput = document.getElementById('search-email');
    if (emailInput) emailInput.value = email;
  }

  const user = getCurrentUser();
  if (user && !email) {
    const emailInput = document.getElementById('search-email');
    if (emailInput) emailInput.value = user.email;
    searchBookings();
  } else if (ref || email) {
    searchBookings();
  }
});
