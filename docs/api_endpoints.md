# CineBook — REST API Specification

All API endpoints communicate using standard JSON payloads over HTTP.

## Base URL
`http://localhost:8080/api`

---

## 1. System & Health Endpoints

### `GET /api/db/status`
Checks live connection to the database engine.
- **Response (200 OK):**
```json
{
  "status": "connected",
  "engine": "SQLite (Local Persistent Relational DB)",
  "is_oracle": false,
  "oracle_target": "localhost:1521/XEPDB1",
  "local_db_path": "cinebook.db"
}
```

---

## 2. Public Movie & Showtime Endpoints

### `GET /api/movies`
Returns movie catalog. Supports optional query parameters.
- **Query Parameters:**
  - `search` (string): Title/director/cast query.
  - `genre` (string): e.g. "Sci-Fi"
  - `language` (string): e.g. "English"
- **Response (200 OK):**
```json
[
  {
    "id": 1,
    "title": "Dune: Part Two",
    "genre": "Sci-Fi",
    "language": "English",
    "duration_mins": 166,
    "release_date": "2024-03-01",
    "certificate": "UA",
    "rating": 8.6,
    "director": "Denis Villeneuve",
    "cast_details": "Timothée Chalamet, Zendaya",
    "poster_url": "https://...",
    "description": "Paul Atreides unites with Chani..."
  }
]
```

### `GET /api/movies/:id`
Retrieves single movie by ID.
- **Response (200 OK):** Movie object.
- **Response (404 Not Found):** `{"error": "Movie not found"}`

### `GET /api/genres`
Returns list of distinct movie genres present in the database.
- **Response (200 OK):** `["Action", "Animation", "Biography", "Sci-Fi"]`

### `GET /api/languages`
Returns list of distinct movie languages.
- **Response (200 OK):** `["English", "Telugu"]`

### `GET /api/shows`
Returns scheduled showtimes.
- **Query Parameters:**
  - `movie_id` (integer): Filter by movie.
  - `date` (string): Filter by date format `YYYY-MM-DD`.
- **Response (200 OK):**
```json
[
  {
    "show_id": 1,
    "show_date": "2026-10-14",
    "show_time": "10:30",
    "movie_id": 1,
    "movie_title": "Dune: Part Two",
    "theatre_name": "PVR Superplex Mall of India",
    "city": "Noida",
    "screen_name": "Screen 1 (IMAX Laser)",
    "total_seats": 25,
    "available_seats": 21
  }
]
```

### `GET /api/shows/:id/seats`
Returns seat map with live booked/available flags for that show.
- **Response (200 OK):**
```json
{
  "show": {
    "show_id": 1,
    "movie_title": "Dune: Part Two",
    "theatre_name": "PVR Superplex Mall of India",
    "screen_name": "Screen 1 (IMAX Laser)",
    "show_date": "2026-10-14",
    "show_time": "10:30"
  },
  "seats": [
    {
      "id": 1,
      "seat_number": "A1",
      "row_name": "A",
      "category": "Regular",
      "base_price": 180.0,
      "is_booked": true
    },
    {
      "id": 6,
      "seat_number": "B1",
      "row_name": "B",
      "category": "Regular",
      "base_price": 180.0,
      "is_booked": false
    }
  ]
}
```

---

## 3. Customer Booking Endpoints

### `POST /api/bookings`
Reserves seats atomically and simulates payment.
- **Request Body:**
```json
{
  "customer_name": "Srijith S D",
  "customer_email": "srijith@college.edu",
  "customer_phone": "9876543210",
  "show_id": 1,
  "seat_ids": [6, 7]
}
```
- **Response (201 Created):**
```json
{
  "success": true,
  "booking_reference": "CB-20261014-9B21",
  "booking_id": 4,
  "movie_title": "Dune: Part Two",
  "theatre_name": "PVR Superplex Mall of India",
  "screen_name": "Screen 1 (IMAX Laser)",
  "show_date": "2026-10-14",
  "show_time": "10:30",
  "ticket_count": 2,
  "total_amount": 360.0,
  "payment_status": "SUCCESS",
  "booking_status": "CONFIRMED",
  "seats": [
    {"seat_id": 6, "seat_number": "B1", "category": "Regular", "price": 180.0},
    {"seat_id": 7, "seat_number": "B2", "category": "Regular", "price": 180.0}
  ]
}
```
- **Response (409 Conflict):** Double-booking prevention:
```json
{
  "error": "Seat B1 is already booked by another user for this show. Please select a different seat."
}
```

### `GET /api/bookings/:reference`
Looks up single booking by reference code. Supports optional `?email=...` check.

### `GET /api/bookings?email=...`
Returns list of all bookings placed by that customer email.

### `POST /api/bookings/:reference/cancel`
Cancels booking, refunds transaction, and releases seats atomically.
- **Response (200 OK):**
```json
{
  "success": true,
  "message": "Booking CB-20261014-9B21 has been cancelled and seats have been released."
}
```

---

## 4. Admin Management Endpoints
*Requires header: `X-Admin-Token: cinebook_admin_secure_key_2026`*

- `GET /api/admin/stats` — Total revenues, booking counts, active screens.
- `POST /api/admin/movies` — Insert new movie.
- `PUT /api/admin/movies/:id` — Update movie catalog data.
- `DELETE /api/admin/movies/:id` — Delete movie (blocked if shows reference it).
- `GET /api/admin/theatres` — List cinemas.
- `POST /api/admin/theatres` — Add new cinema.
- `GET /api/admin/screens` — List screens.
- `POST /api/admin/screens` — Add screen (auto-generates 25 seats).
- `GET /api/admin/shows` — List scheduled shows.
- `POST /api/admin/shows` — Schedule show (checks time conflicts).
- `DELETE /api/admin/shows/:id` — Delete show (blocked if confirmed bookings exist).
- `GET /api/admin/bookings` — View all customer reservations.
- `PUT /api/admin/bookings/:reference/status` — Update booking status.
