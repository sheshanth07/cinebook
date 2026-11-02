# CineBook — College Viva Voce Preparation Guide

This guide prepares you to confidently answer questions from professors and external examiners during your Database Systems Design mini-project viva.

---

## 1. Project Pitch (30-Second Summary)
> *"CineBook is a full-stack movie ticket booking system built for our Database Systems Design curriculum. It features an HTML5/CSS3 frontend, a custom lightweight C++ HTTP server, and a fully normalized relational database schema (8 entities in 3NF). It prevents concurrent double-booking of seats using atomic database transactions and unique constraints. It provides two database drivers: native Oracle Database connectivity (via OCCI) with complete Oracle DDL scripts, and a local persistent SQL engine for self-contained demonstration."*

---

## 2. Key Modules & Technical Roles

1. **Frontend Module (`/frontend`):**
   - Built with vanilla HTML5, CSS3, and JavaScript (no heavy node/react runtime required).
   - Dynamically loads movies, schedules, and seat matrices from the C++ REST backend.
   - Live seat selection calculates totals and revalidates seat availability with the database before submitting payment.

2. **C++ HTTP Server (`http_server.cpp`):**
   - High-performance, lightweight multithreaded HTTP server using `cpp-httplib`.
   - Serves static assets and routes REST API requests (`/api/movies`, `/api/shows`, `/api/bookings`, `/api/admin/*`).
   - Parses and serializes JSON with `nlohmann::json`.

3. **Database Abstraction Layer (`database.hpp` / `database.cpp`):**
   - Defines abstract `IDatabase` interface.
   - Parameterized SQL execution preventing SQL injection attacks.
   - Dual driver support: Oracle Database (OCCI) and local persistent SQL engine (`cinebook.db`).

4. **Booking Service & Transaction Coordinator (`booking_service.cpp`):**
   - Implements ACID transaction boundaries (`BEGIN TRANSACTION` -> check unbooked seats -> insert `BOOKINGS` -> insert `BOOKING_SEATS` -> `COMMIT`).
   - If any seat is already booked, it triggers an immediate `ROLLBACK` and returns HTTP 409 Conflict.

5. **Admin Service (`admin_service.cpp`):**
   - Guards relational referential integrity.
   - Prevents deletion of movies with scheduled shows or shows with confirmed bookings.

---

## 3. Frequently Asked Viva Questions & Model Answers

### Q1: What normalization level is your schema in?
**Answer:**
The schema is in **Third Normal Form (3NF)**:
- **1NF:** Every column has atomic (indivisible) values, and every table has a unique primary key. Repeating seat selections are extracted into a junction table `BOOKING_SEATS`.
- **2NF:** It is in 1NF and has no partial dependencies (every non-key attribute depends on the whole primary key).
- **3NF:** It has no transitive dependencies. For example, Theatre details (`city`, `location`) are not stored inside `SHOWS` or `BOOKINGS`; instead, foreign keys navigate to `THEATRES`. Seat price at time of purchase is snapshotted into `BOOKING_SEATS` so future price changes do not distort historical accounting.

### Q2: How do you prevent double-booking if two users select the same seat at the same second?
**Answer:**
We employ a **two-tier concurrency protection strategy**:
1. **Application-level Transaction Check:** Inside a database transaction (`BEGIN TRANSACTION`), the backend verifies whether `status = 'BOOKED'` exists in `BOOKING_SEATS` for that `show_id` and `seat_id`. If found, the transaction is rolled back and an HTTP 409 error is returned.
2. **Database-level Constraint:** A unique partial index/constraint `UQ_ACTIVE_SHOW_SEAT` on `(show_id, seat_id)` where `status = 'BOOKED'` exists at the RDBMS engine layer. Even if two threads execute concurrently, the database engine enforces serialization and rejects the duplicate insert.

### Q3: Why does your schema separate `BOOKINGS` and `BOOKING_SEATS`?
**Answer:**
A single customer order can book multiple seats (a one-to-many relationship). `BOOKINGS` stores the transaction-level metadata (user ID, show ID, booking reference, total payment, timestamp), while `BOOKING_SEATS` stores individual seat allocations and historical price snapshots. This eliminates multivalued columns and satisfies First Normal Form (1NF).

### Q4: What happens when a booking is cancelled?
**Answer:**
We execute an atomic transaction:
1. `BOOKINGS.booking_status` is updated to `'CANCELLED'` and `payment_status` to `'REFUNDED'`.
2. `BOOKING_SEATS.status` is updated to `'CANCELLED'` for that booking reference.
3. Because the unique seat constraint only filters on `status = 'BOOKED'`, changing the status to `'CANCELLED'` immediately releases the seat so other customers can book it.

### Q5: What is the difference between Oracle SQL and SQLite in your project?
**Answer:**
- **Oracle Database:** Enterprise client-server RDBMS. Uses Oracle syntax such as `NUMBER GENERATED AS IDENTITY`, `VARCHAR2`, `TIMESTAMP`, and `TO_DATE()`. Connects via Oracle C++ Call Interface (OCCI). Our `database/schema.sql` and `database/sample_data.sql` are written specifically for Oracle 19c/21c/XE.
- **Local Engine (SQLite):** An embedded serverless SQL engine compiled directly into our C++ binary. It provides a persistent database file (`cinebook.db`) with identical tables, foreign keys (`PRAGMA foreign_keys = ON`), constraints, and transactions without requiring external server setup for evaluation.

### Q6: How do you prevent SQL Injection?
**Answer:**
We use **parameterized prepared statements** (`sqlite3_prepare_v2` / OCCI bind variables). User inputs are bound as parameters (`?` placeholders) rather than concatenated into SQL strings. This ensures input data is treated strictly as literals, preventing code execution through input fields.

### Q7: Explain the two database views you created.
**Answer:**
1. `VW_MOVIE_SHOWTIMES`: Joins `MOVIES`, `SHOWS`, `SCREENS`, and `THEATRES` with a correlated subquery counting available seats. This abstracts complex multi-table joins for fast frontend showtime queries.
2. `VW_BOOKING_REPORTS`: Joins `BOOKINGS`, `USERS`, `SHOWS`, and `MOVIES` with `LISTAGG` / seat grouping to produce customer ticket audit and financial reporting.
