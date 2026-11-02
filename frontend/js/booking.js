/**
 * CineBook - Order Checkout & Payment Simulation Script
 */

let pendingOrder = null;

function loadOrderDetails() {
  const user = getCurrentUser();
  if (!user) {
    showToast('Ticket booking requires logging in. Redirecting to login...', 'error');
    setTimeout(() => {
      window.location.href = 'login.html?redirect=booking.html&msg=login_required';
    }, 1000);
    return;
  }

  const stored = sessionStorage.getItem('cinebook_pending_order');
  if (!stored) {
    showToast('No active booking order found.', 'error');
    setTimeout(() => window.location.href = 'movies.html', 1500);
    return;
  }

  pendingOrder = JSON.parse(stored);
  renderOrderSummary(pendingOrder);

  // Auto-fill logged in user credentials
  const nameInput = document.getElementById('cust-name');
  const emailInput = document.getElementById('cust-email');
  if (nameInput) {
    nameInput.value = user.username;
    nameInput.readOnly = true;
    nameInput.style.backgroundColor = 'rgba(255, 255, 255, 0.05)';
  }
  if (emailInput) {
    emailInput.value = user.email;
    emailInput.readOnly = true;
    emailInput.style.backgroundColor = 'rgba(255, 255, 255, 0.05)';
  }
}

function renderOrderSummary(order) {
  const container = document.getElementById('order-summary-content');
  if (!container) return;

  container.innerHTML = `
    <div style="display: flex; gap: 1.25rem; margin-bottom: 1.5rem; align-items: center;">
      <img src="${order.poster_url}" alt="${order.movie_title}" 
           style="width: 65px; height: 95px; object-fit: cover; border-radius: var(--radius-sm);" 
           onerror="this.onerror=null; this.src='https://images.unsplash.com/photo-1489599849927-2ee91cede3ba?w=600';" />
      <div>
        <h3 style="font-size: 1.3rem; margin-bottom: 0.25rem;">${order.movie_title}</h3>
        <p style="color: var(--text-muted); font-size: 0.9rem;">${order.theatre_name}</p>
        <p style="color: var(--accent-cyan); font-size: 0.85rem; font-weight: 600;">${order.screen_name}</p>
      </div>
    </div>

    <div style="border-top: 1px solid var(--border-color); border-bottom: 1px solid var(--border-color); padding: 1rem 0; margin-bottom: 1.5rem; font-size: 0.9rem;">
      <div style="display: flex; justify-content: space-between; margin-bottom: 0.5rem;">
        <span style="color: var(--text-muted);">Date & Time:</span>
        <span style="font-weight: 600;">${order.show_date} • ${order.show_time}</span>
      </div>
      <div style="display: flex; justify-content: space-between; margin-bottom: 0.5rem;">
        <span style="color: var(--text-muted);">Seats Selected:</span>
        <span style="font-weight: 600; color: var(--accent-gold);">${order.seat_names.join(', ')}</span>
      </div>
      <div style="display: flex; justify-content: space-between;">
        <span style="color: var(--text-muted);">Ticket Count:</span>
        <span style="font-weight: 600;">${order.seat_ids.length} Ticket(s)</span>
      </div>
    </div>

    <div style="display: flex; justify-content: space-between; align-items: center; font-size: 1.2rem; font-weight: 800; margin-bottom: 1.5rem;">
      <span>Total Payable:</span>
      <span style="color: #22c55e;">${formatCurrency(order.total_amount)}</span>
    </div>
  `;
}

async function handleBookingSubmission(e) {
  e.preventDefault();

  if (!pendingOrder) {
    showToast('Missing booking details.', 'error');
    return;
  }

  const nameInput = document.getElementById('cust-name');
  const emailInput = document.getElementById('cust-email');
  const phoneInput = document.getElementById('cust-phone');
  const submitBtn = document.getElementById('pay-btn');

  const customerName = nameInput.value.trim();
  const customerEmail = emailInput.value.trim();
  const customerPhone = phoneInput.value.trim();

  if (!customerName || !customerEmail) {
    showToast('Please enter your full name and email.', 'error');
    return;
  }

  submitBtn.disabled = true;
  submitBtn.innerHTML = 'Processing Transaction & Reserving Seats...';

  try {
    const payload = {
      customer_name: customerName,
      customer_email: customerEmail,
      customer_phone: customerPhone,
      show_id: pendingOrder.show_id,
      seat_ids: pendingOrder.seat_ids
    };

    const res = await fetch(`${API_BASE}/api/bookings`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });

    const data = await res.json();

    if (!res.ok) {
      // Handles 409 Conflict (Double booking prevention) or 400 Bad Request
      throw new Error(data.error || 'Booking transaction failed.');
    }

    // Success! Clear session storage
    sessionStorage.removeItem('cinebook_pending_order');
    showBookingConfirmation(data);

  } catch (err) {
    console.error(err);
    showToast(err.message, 'error');
    submitBtn.disabled = false;
    submitBtn.innerHTML = 'Confirm Booking & Simulate Payment';
  }
}

function showBookingConfirmation(receipt) {
  const checkoutSection = document.getElementById('checkout-section');
  const successSection = document.getElementById('success-section');

  if (checkoutSection) checkoutSection.style.display = 'none';
  if (successSection) {
    successSection.style.display = 'block';
    successSection.innerHTML = `
      <div class="card" style="max-width: 650px; margin: 0 auto; text-align: center; padding: 3rem 2rem; border-color: rgba(34, 197, 94, 0.4);">
        <div style="width: 70px; height: 70px; background: rgba(34, 197, 94, 0.15); color: #22c55e; border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 2.5rem; margin: 0 auto 1.5rem auto;">
          ✓
        </div>
        <h2 style="font-size: 2rem; margin-bottom: 0.5rem; color: #ffffff;">Booking Confirmed!</h2>
        <p style="color: var(--text-muted); font-size: 0.95rem; margin-bottom: 2rem;">
          Your seats are permanently reserved in the database. A booking reference has been issued.
        </p>

        <div style="background: var(--bg-card); border: 1px dashed var(--border-color); border-radius: var(--radius-md); padding: 1.5rem; text-align: left; margin-bottom: 2rem;">
          <div style="display: flex; justify-content: space-between; margin-bottom: 0.75rem; border-bottom: 1px solid var(--border-light); padding-bottom: 0.75rem;">
            <span style="color: var(--text-muted);">Booking Reference:</span>
            <span style="font-weight: 800; font-size: 1.1rem; color: var(--accent-gold); letter-spacing: 1px;">
              ${receipt.booking_reference}
            </span>
          </div>
          <div style="display: flex; justify-content: space-between; margin-bottom: 0.5rem;">
            <span style="color: var(--text-muted);">Movie:</span>
            <span style="font-weight: 600;">${receipt.movie_title}</span>
          </div>
          <div style="display: flex; justify-content: space-between; margin-bottom: 0.5rem;">
            <span style="color: var(--text-muted);">Cinema:</span>
            <span>${receipt.theatre_name} (${receipt.screen_name})</span>
          </div>
          <div style="display: flex; justify-content: space-between; margin-bottom: 0.5rem;">
            <span style="color: var(--text-muted);">Schedule:</span>
            <span>${receipt.show_date} at ${receipt.show_time}</span>
          </div>
          <div style="display: flex; justify-content: space-between; margin-bottom: 0.5rem;">
            <span style="color: var(--text-muted);">Reserved Seats:</span>
            <span style="font-weight: 700; color: #4ade80;">
              ${receipt.seats.map(s => s.seat_number).join(', ')}
            </span>
          </div>
          <div style="display: flex; justify-content: space-between; margin-top: 0.75rem; border-top: 1px solid var(--border-light); padding-top: 0.75rem;">
            <span style="color: var(--text-muted);">Amount Paid:</span>
            <span style="font-weight: 800; font-size: 1.1rem; color: #22c55e;">
              ${formatCurrency(receipt.total_amount)}
            </span>
          </div>
        </div>

        <div style="display: flex; gap: 1rem; justify-content: center; flex-wrap: wrap;">
          <button class="btn btn-secondary" onclick="window.print()">Print Ticket</button>
          <a href="my-bookings.html?ref=${receipt.booking_reference}&email=${encodeURIComponent(receipt.customer_email)}" class="btn btn-primary">
            View in My Bookings
          </a>
          <a href="index.html" class="btn btn-secondary">Home</a>
        </div>
      </div>
    `;
  }
}

document.addEventListener('DOMContentLoaded', () => {
  loadOrderDetails();

  const form = document.getElementById('booking-form');
  if (form) {
    form.addEventListener('submit', handleBookingSubmission);
  }
});
