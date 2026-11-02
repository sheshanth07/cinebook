-- ============================================================================
-- CineBook - Movie Ticket Booking System
-- Database Systems Design Mini-Project
-- File: database/sample_data.sql
-- Target Database: Oracle Database (12c / 18c / 19c / 21c / 23ai / XE)
-- ============================================================================

-- Clean up existing data before inserting
DELETE FROM BOOKING_SEATS;
DELETE FROM BOOKINGS;
DELETE FROM SHOWS;
DELETE FROM SEATS;
DELETE FROM SCREENS;
DELETE FROM THEATRES;
DELETE FROM MOVIES;
DELETE FROM USERS;

-- ----------------------------------------------------------------------------
-- 1. SEED USERS
-- Password hashes use SHA-256:
-- 'admin123' -> 240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9
-- 'user123'  -> e606e38b0d8c19b24cf0ee3808183162ea7cd63ff7912dbb22b5e803286b4446
-- ----------------------------------------------------------------------------
INSERT INTO USERS (id, username, email, password_hash, role, created_at)
VALUES (1, 'admin', 'admin@cinebook.com', '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9', 'admin', CURRENT_TIMESTAMP);

INSERT INTO USERS (id, username, email, password_hash, role, created_at)
VALUES (2, 'rahul_sharma', 'rahul@example.com', 'e606e38b0d8c19b24cf0ee3808183162ea7cd63ff7912dbb22b5e803286b4446', 'user', CURRENT_TIMESTAMP);

INSERT INTO USERS (id, username, email, password_hash, role, created_at)
VALUES (3, 'priya_patel', 'priya@example.com', 'e606e38b0d8c19b24cf0ee3808183162ea7cd63ff7912dbb22b5e803286b4446', 'user', CURRENT_TIMESTAMP);

INSERT INTO USERS (id, username, email, password_hash, role, created_at)
VALUES (4, 'alex_turner', 'alex@example.com', 'e606e38b0d8c19b24cf0ee3808183162ea7cd63ff7912dbb22b5e803286b4446', 'user', CURRENT_TIMESTAMP);

-- ----------------------------------------------------------------------------
-- 2. SEED MOVIES
-- ----------------------------------------------------------------------------
INSERT INTO MOVIES (id, title, description, genre, language, duration_mins, release_date, certificate, rating, cast_details, director, poster_url, banner_url, is_upcoming)
VALUES (
    1, 
    'Dune: Part Two', 
    'Paul Atreides unites with Chani and the Fremen while seeking revenge against the conspirators who destroyed his family.', 
    'Sci-Fi', 
    'English', 
    166, 
    TO_DATE('2024-03-01', 'YYYY-MM-DD'), 
    'UA', 
    8.6, 
    'Timothée Chalamet, Zendaya, Rebecca Ferguson, Javier Bardem', 
    'Denis Villeneuve', 
    'https://images.unsplash.com/photo-1534447677768-be436bb09401?w=600&auto=format&fit=crop&q=80', 
    'https://images.unsplash.com/photo-1509198397868-475647b2a1e5?w=1200&auto=format&fit=crop&q=80', 
    0
);

INSERT INTO MOVIES (id, title, description, genre, language, duration_mins, release_date, certificate, rating, cast_details, director, poster_url, banner_url, is_upcoming)
VALUES (
    2, 
    'Oppenheimer', 
    'The story of American scientist J. Robert Oppenheimer and his role in the development of the atomic bomb during World War II.', 
    'Biography', 
    'English', 
    180, 
    TO_DATE('2023-07-21', 'YYYY-MM-DD'), 
    'A', 
    8.9, 
    'Cillian Murphy, Emily Blunt, Matt Damon, Robert Downey Jr.', 
    'Christopher Nolan', 
    'https://images.unsplash.com/photo-1440404653325-ab127d49abc1?w=600&auto=format&fit=crop&q=80', 
    'https://images.unsplash.com/photo-1478760329108-5c3ed9d495a0?w=1200&auto=format&fit=crop&q=80', 
    0
);

INSERT INTO MOVIES (id, title, description, genre, language, duration_mins, release_date, certificate, rating, cast_details, director, poster_url, banner_url, is_upcoming)
VALUES (
    3, 
    'Inception', 
    'A thief who steals corporate secrets through the use of dream-sharing technology is given the inverse task of planting an idea.', 
    'Sci-Fi', 
    'English', 
    148, 
    TO_DATE('2010-07-16', 'YYYY-MM-DD'), 
    'UA', 
    8.8, 
    'Leonardo DiCaprio, Joseph Gordon-Levitt, Elliot Page, Tom Hardy', 
    'Christopher Nolan', 
    'https://images.unsplash.com/photo-1489599849927-2ee91cede3ba?w=600&auto=format&fit=crop&q=80', 
    'https://images.unsplash.com/photo-1518709268805-4e9042af9f23?w=1200&auto=format&fit=crop&q=80', 
    0
);

INSERT INTO MOVIES (id, title, description, genre, language, duration_mins, release_date, certificate, rating, cast_details, director, poster_url, banner_url, is_upcoming)
VALUES (
    4, 
    'RRR (Rise Roar Revolt)', 
    'A fearless warrior on a perilous mission confronts a charismatic British police officer in 1920s pre-independent India.', 
    'Action', 
    'Telugu', 
    187, 
    TO_DATE('2022-03-25', 'YYYY-MM-DD'), 
    'UA', 
    8.1, 
    'N.T. Rama Rao Jr., Ram Charan, Ajay Devgn, Alia Bhatt', 
    'S.S. Rajamouli', 
    'https://images.unsplash.com/photo-1579783902614-a3fb3927b675?w=600&auto=format&fit=crop&q=80', 
    'https://images.unsplash.com/photo-1514306191717-452ec28c7814?w=1200&auto=format&fit=crop&q=80', 
    0
);

INSERT INTO MOVIES (id, title, description, genre, language, duration_mins, release_date, certificate, rating, cast_details, director, poster_url, banner_url, is_upcoming)
VALUES (
    5, 
    'Spider-Man: Across the Spider-Verse', 
    'Miles Morales catapults across the Multiverse, where he encounters a team of Spider-People charged with protecting its very existence.', 
    'Animation', 
    'English', 
    140, 
    TO_DATE('2023-06-02', 'YYYY-MM-DD'), 
    'U', 
    8.7, 
    'Shameik Moore, Hailee Steinfeld, Oscar Isaac, Jake Johnson', 
    'Joaquim Dos Santos, Kemp Powers', 
    'https://images.unsplash.com/photo-1635805737707-575885ab0820?w=600&auto=format&fit=crop&q=80', 
    'https://images.unsplash.com/photo-1534447677768-be436bb09401?w=1200&auto=format&fit=crop&q=80', 
    0
);

INSERT INTO MOVIES (id, title, description, genre, language, duration_mins, release_date, certificate, rating, cast_details, director, poster_url, banner_url, is_upcoming)
VALUES (
    6, 
    'Interstellar', 
    'When Earth becomes uninhabitable in the future, a farmer and ex-NASA pilot is tasked to pilot a spacecraft along with a team of researchers.', 
    'Sci-Fi', 
    'English', 
    169, 
    TO_DATE('2014-11-07', 'YYYY-MM-DD'), 
    'UA', 
    8.7, 
    'Matthew McConaughey, Anne Hathaway, Jessica Chastain, Michael Caine', 
    'Christopher Nolan', 
    'https://images.unsplash.com/photo-1451187580459-43490279c0fa?w=600&auto=format&fit=crop&q=80', 
    'https://images.unsplash.com/photo-1446776811953-b23d57bd21aa?w=1200&auto=format&fit=crop&q=80', 
    0
);

INSERT INTO MOVIES (id, title, description, genre, language, duration_mins, release_date, certificate, rating, cast_details, director, poster_url, banner_url, is_upcoming)
VALUES (
    7,
    'Doraemon: Nobita''s Sky Utopia',
    'Nobita and Doraemon embark on an extraordinary journey through time and space in a time airship to find Utopia, a legendary crescent-shaped island in the sky.',
    'Animation',
    'Japanese',
    108,
    TO_DATE('2023-03-03', 'YYYY-MM-DD'),
    'U',
    8.2,
    'Wasabi Mizuta, Megumi Oohara, Yumi Kakazu, Tomokazu Seki',
    'Takumi Doyama',
    'https://images.unsplash.com/photo-1578632767115-351597cf2477?w=600&auto=format&fit=crop&q=80',
    'https://images.unsplash.com/photo-1534447677768-be436bb09401?w=1200&auto=format&fit=crop&q=80',
    0
);

INSERT INTO MOVIES (id, title, description, genre, language, duration_mins, release_date, certificate, rating, cast_details, director, poster_url, banner_url, is_upcoming)
VALUES (
    8,
    'Jailer 2',
    'Tiger Muthuvel Pandian returns in this high-octane action thriller, facing a dangerous international syndicate that threatens law and order.',
    'Action',
    'Tamil',
    168,
    TO_DATE('2025-08-10', 'YYYY-MM-DD'),
    'UA',
    8.9,
    'Rajinikanth, Mohanlal, Shiva Rajkumar, Ramya Krishnan',
    'Nelson Dilipkumar',
    'https://images.unsplash.com/photo-1514306191717-452ec28c7814?w=600&auto=format&fit=crop&q=80',
    'https://images.unsplash.com/photo-1509198397868-475647b2a1e5?w=1200&auto=format&fit=crop&q=80',
    0
);

-- ----------------------------------------------------------------------------
-- 3. SEED THEATRES
-- ----------------------------------------------------------------------------
INSERT INTO THEATRES (id, name, location, city)
VALUES (1, 'PVR Superplex Mall of India', 'Sector 18', 'Noida');

INSERT INTO THEATRES (id, name, location, city)
VALUES (2, 'INOX Megaplex Forum', 'Koramangala', 'Bengaluru');

INSERT INTO THEATRES (id, name, location, city)
VALUES (3, 'Cinepolis Grand Central', 'Seawoods', 'Navi Mumbai');

-- ----------------------------------------------------------------------------
-- 4. SEED SCREENS
-- ----------------------------------------------------------------------------
INSERT INTO SCREENS (id, theatre_id, screen_name, total_seats)
VALUES (1, 1, 'Screen 1 (IMAX Laser)', 25);

INSERT INTO SCREENS (id, theatre_id, screen_name, total_seats)
VALUES (2, 1, 'Screen 2 (Dolby Atmos)', 25);

INSERT INTO SCREENS (id, theatre_id, screen_name, total_seats)
VALUES (3, 2, 'Audi 1 (Koramangala 4DX)', 25);

INSERT INTO SCREENS (id, theatre_id, screen_name, total_seats)
VALUES (4, 3, 'Screen 3 (Cinepolis VIP)', 25);

-- ----------------------------------------------------------------------------
-- 5. SEED SEATS
-- 25 seats per screen layout:
-- Rows A, B, C (5 seats each) = Regular ($150 - $200)
-- Rows D, E (5 seats each) = Premium ($280 - $350)
-- ----------------------------------------------------------------------------
-- Screen 1 Seats
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'A1', 'A', 'Regular', 180.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'A2', 'A', 'Regular', 180.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'A3', 'A', 'Regular', 180.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'A4', 'A', 'Regular', 180.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'A5', 'A', 'Regular', 180.00);

INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'B1', 'B', 'Regular', 180.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'B2', 'B', 'Regular', 180.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'B3', 'B', 'Regular', 180.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'B4', 'B', 'Regular', 180.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'B5', 'B', 'Regular', 180.00);

INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'C1', 'C', 'Regular', 200.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'C2', 'C', 'Regular', 200.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'C3', 'C', 'Regular', 200.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'C4', 'C', 'Regular', 200.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'C5', 'C', 'Regular', 200.00);

INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'D1', 'D', 'Premium', 300.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'D2', 'D', 'Premium', 300.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'D3', 'D', 'Premium', 300.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'D4', 'D', 'Premium', 300.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'D5', 'D', 'Premium', 300.00);

INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'E1', 'E', 'Premium', 350.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'E2', 'E', 'Premium', 350.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'E3', 'E', 'Premium', 350.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'E4', 'E', 'Premium', 350.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (1, 'E5', 'E', 'Premium', 350.00);

-- Screen 2 Seats
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'A1', 'A', 'Regular', 160.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'A2', 'A', 'Regular', 160.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'A3', 'A', 'Regular', 160.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'A4', 'A', 'Regular', 160.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'A5', 'A', 'Regular', 160.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'B1', 'B', 'Regular', 160.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'B2', 'B', 'Regular', 160.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'B3', 'B', 'Regular', 160.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'B4', 'B', 'Regular', 160.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'B5', 'B', 'Regular', 160.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'C1', 'C', 'Regular', 190.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'C2', 'C', 'Regular', 190.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'C3', 'C', 'Regular', 190.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'C4', 'C', 'Regular', 190.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'C5', 'C', 'Regular', 190.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'D1', 'D', 'Premium', 280.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'D2', 'D', 'Premium', 280.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'D3', 'D', 'Premium', 280.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'D4', 'D', 'Premium', 280.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'D5', 'D', 'Premium', 280.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'E1', 'E', 'Premium', 320.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'E2', 'E', 'Premium', 320.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'E3', 'E', 'Premium', 320.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'E4', 'E', 'Premium', 320.00);
INSERT INTO SEATS (screen_id, seat_number, row_name, category, base_price) VALUES (2, 'E5', 'E', 'Premium', 320.00);

-- ----------------------------------------------------------------------------
-- 6. SEED SHOWS
-- Dynamic TRUNC(SYSDATE) ensures shows are always active for today & tomorrow
-- ----------------------------------------------------------------------------
INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time)
VALUES (1, 1, 1, TRUNC(SYSDATE), '10:30');

INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time)
VALUES (2, 1, 1, TRUNC(SYSDATE), '14:30');

INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time)
VALUES (3, 1, 1, TRUNC(SYSDATE), '18:45');

INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time)
VALUES (4, 2, 2, TRUNC(SYSDATE), '11:00');

INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time)
VALUES (5, 2, 2, TRUNC(SYSDATE), '15:30');

INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time)
VALUES (6, 2, 2, TRUNC(SYSDATE), '19:45');

INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time)
VALUES (7, 3, 1, TRUNC(SYSDATE) + 1, '14:00');

INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time)
VALUES (8, 4, 2, TRUNC(SYSDATE) + 1, '18:00');

INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time)
VALUES (9, 5, 1, TRUNC(SYSDATE) + 1, '20:30');

INSERT INTO SHOWS (id, movie_id, screen_id, show_date, show_time)
VALUES (10, 6, 2, TRUNC(SYSDATE) + 2, '17:15');

-- ----------------------------------------------------------------------------
-- 7. SEED BOOKINGS
-- Sample customer bookings
-- ----------------------------------------------------------------------------
INSERT INTO BOOKINGS (id, booking_reference, user_id, show_id, total_amount, ticket_count, payment_status, booking_status, created_at)
VALUES (1, 'CB-20261014-8801', 2, 1, 360.00, 2, 'SUCCESS', 'CONFIRMED', CURRENT_TIMESTAMP);

INSERT INTO BOOKINGS (id, booking_reference, user_id, show_id, total_amount, ticket_count, payment_status, booking_status, created_at)
VALUES (2, 'CB-20261014-9914', 3, 1, 650.00, 2, 'SUCCESS', 'CONFIRMED', CURRENT_TIMESTAMP);

INSERT INTO BOOKINGS (id, booking_reference, user_id, show_id, total_amount, ticket_count, payment_status, booking_status, created_at)
VALUES (3, 'CB-20261014-1102', 4, 3, 300.00, 1, 'SUCCESS', 'CONFIRMED', CURRENT_TIMESTAMP);

-- ----------------------------------------------------------------------------
-- 8. SEED BOOKING_SEATS
-- ----------------------------------------------------------------------------
-- Booking 1 booked seats A1 and A2 for Show 1
INSERT INTO BOOKING_SEATS (booking_id, show_id, seat_id, price_snapshot, status)
VALUES (1, 1, 1, 180.00, 'BOOKED'); -- A1

INSERT INTO BOOKING_SEATS (booking_id, show_id, seat_id, price_snapshot, status)
VALUES (1, 1, 2, 180.00, 'BOOKED'); -- A2

-- Booking 2 booked seats D1 (300) and E1 (350) for Show 1
INSERT INTO BOOKING_SEATS (booking_id, show_id, seat_id, price_snapshot, status)
VALUES (2, 1, 16, 300.00, 'BOOKED'); -- D1

INSERT INTO BOOKING_SEATS (booking_id, show_id, seat_id, price_snapshot, status)
VALUES (2, 1, 21, 350.00, 'BOOKED'); -- E1

-- Booking 3 booked seat D2 for Show 3
INSERT INTO BOOKING_SEATS (booking_id, show_id, seat_id, price_snapshot, status)
VALUES (3, 3, 17, 300.00, 'BOOKED'); -- D2

COMMIT;
