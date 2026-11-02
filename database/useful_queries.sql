-- ============================================================================
-- CineBook - Movie Ticket Booking System
-- Database Systems Design Mini-Project
-- File: database/useful_queries.sql
-- Description: Practical queries demonstrating core RDBMS concepts for viva
-- ============================================================================

-- ============================================================================
-- SECTION 1: DATABASE VIEWS
-- ============================================================================

-- VIEW 1: Movie Showtimes View
-- Denormalizes movie, theatre, screen, and show details for fast schedule lookups
CREATE OR REPLACE VIEW VW_MOVIE_SHOWTIMES AS
SELECT 
    s.id AS show_id,
    m.id AS movie_id,
    m.title AS movie_title,
    m.language,
    m.genre,
    m.certificate,
    m.duration_mins,
    m.rating,
    t.id AS theatre_id,
    t.name AS theatre_name,
    t.city AS theatre_city,
    sc.id AS screen_id,
    sc.screen_name,
    TO_CHAR(s.show_date, 'YYYY-MM-DD') AS formatted_show_date,
    s.show_date,
    s.show_time,
    sc.total_seats,
    (sc.total_seats - (
        SELECT COUNT(*) 
        FROM BOOKING_SEATS bs 
        WHERE bs.show_id = s.id AND bs.status = 'BOOKED'
    )) AS available_seats
FROM SHOWS s
INNER JOIN MOVIES m ON s.movie_id = m.id
INNER JOIN SCREENS sc ON s.screen_id = sc.id
INNER JOIN THEATRES t ON sc.theatre_id = t.id;

-- VIEW 2: Booking Reports View
-- Summarizes customer bookings with user, movie, showtime, and seat details
CREATE OR REPLACE VIEW VW_BOOKING_REPORTS AS
SELECT 
    b.id AS booking_id,
    b.booking_reference,
    u.id AS user_id,
    u.username,
    u.email AS user_email,
    m.title AS movie_title,
    t.name AS theatre_name,
    sc.screen_name,
    TO_CHAR(s.show_date, 'YYYY-MM-DD') AS show_date,
    s.show_time,
    b.ticket_count,
    b.total_amount,
    b.payment_status,
    b.booking_status,
    b.created_at,
    (
        SELECT LISTAGG(st.seat_number, ', ') WITHIN GROUP (ORDER BY st.seat_number)
        FROM BOOKING_SEATS bs
        JOIN SEATS st ON bs.seat_id = st.id
        WHERE bs.booking_id = b.id
    ) AS booked_seat_numbers
FROM BOOKINGS b
INNER JOIN USERS u ON b.user_id = u.id
INNER JOIN SHOWS s ON b.show_id = s.id
INNER JOIN MOVIES m ON s.movie_id = m.id
INNER JOIN SCREENS sc ON s.screen_id = sc.id
INNER JOIN THEATRES t ON sc.theatre_id = t.id;


-- ============================================================================
-- SECTION 2: DEMONSTRATION QUERIES (FOR LAB & VIVA)
-- ============================================================================

-- 1. SELECT and WHERE
-- Find all English movies with a rating greater than 8.5
SELECT id, title, genre, language, rating, duration_mins 
FROM MOVIES
WHERE language = 'English' AND rating >= 8.5
ORDER BY rating DESC;

-- 2. INNER JOIN
-- Retrieve all scheduled shows today with movie title, screen name, and theatre location
SELECT 
    s.id AS show_id,
    m.title,
    t.name AS theatre,
    t.city,
    sc.screen_name,
    s.show_time
FROM SHOWS s
INNER JOIN MOVIES m ON s.movie_id = m.id
INNER JOIN SCREENS sc ON s.screen_id = sc.id
INNER JOIN THEATRES t ON sc.theatre_id = t.id
WHERE s.show_date >= TRUNC(SYSDATE)
ORDER BY s.show_date ASC, s.show_time ASC;

-- 3. LEFT JOIN
-- List all movies and their total scheduled shows (including movies that currently have zero shows)
SELECT 
    m.id,
    m.title,
    m.genre,
    COUNT(s.id) AS total_scheduled_shows
FROM MOVIES m
LEFT JOIN SHOWS s ON m.id = s.movie_id
GROUP BY m.id, m.title, m.genre
ORDER BY total_scheduled_shows DESC;

-- 4. SUBQUERY (Correlated Subquery)
-- Find movies whose box office revenue is higher than the average revenue across all movies
SELECT 
    m.id,
    m.title,
    NVL(SUM(b.total_amount), 0) AS movie_revenue
FROM MOVIES m
JOIN SHOWS s ON m.id = s.movie_id
JOIN BOOKINGS b ON s.id = b.show_id AND b.booking_status = 'CONFIRMED'
GROUP BY m.id, m.title
HAVING SUM(b.total_amount) > (
    SELECT AVG(total_amount) 
    FROM BOOKINGS 
    WHERE booking_status = 'CONFIRMED'
);

-- 5. SUBQUERY with NOT IN / NOT EXISTS
-- Find all seats in Screen 1 that are currently UNBOOKED for Show ID 1
SELECT s.id, s.seat_number, s.row_name, s.category, s.base_price
FROM SEATS s
WHERE s.screen_id = 1
  AND s.id NOT IN (
      SELECT bs.seat_id
      FROM BOOKING_SEATS bs
      WHERE bs.show_id = 1 AND bs.status = 'BOOKED'
  )
ORDER BY s.row_name, s.seat_number;

-- 6. AGGREGATES: COUNT, SUM, AVG with GROUP BY & ORDER BY
-- Comprehensive theatre performance report: Total shows, total tickets sold, gross revenue, and avg ticket price
SELECT 
    t.name AS theatre_name,
    t.city,
    COUNT(DISTINCT s.id) AS total_shows,
    NVL(SUM(b.ticket_count), 0) AS tickets_sold,
    NVL(SUM(b.total_amount), 0) AS total_revenue,
    NVL(ROUND(AVG(b.total_amount / NULLIF(b.ticket_count, 0)), 2), 0) AS avg_ticket_price
FROM THEATRES t
JOIN SCREENS sc ON t.id = sc.theatre_id
JOIN SHOWS s ON sc.id = s.screen_id
LEFT JOIN BOOKINGS b ON s.id = b.show_id AND b.booking_status = 'CONFIRMED'
GROUP BY t.id, t.name, t.city
ORDER BY total_revenue DESC;

-- 7. OCCUPANCY RATE QUERY
-- Calculate seat occupancy percentage per show
SELECT 
    s.id AS show_id,
    m.title AS movie_title,
    s.show_time,
    sc.total_seats,
    COUNT(bs.id) AS booked_seats,
    ROUND((COUNT(bs.id) / sc.total_seats) * 100, 2) AS occupancy_percentage
FROM SHOWS s
JOIN MOVIES m ON s.movie_id = m.id
JOIN SCREENS sc ON s.screen_id = sc.id
LEFT JOIN BOOKING_SEATS bs ON s.id = bs.show_id AND bs.status = 'BOOKED'
GROUP BY s.id, m.title, s.show_time, sc.total_seats
ORDER BY occupancy_percentage DESC;
