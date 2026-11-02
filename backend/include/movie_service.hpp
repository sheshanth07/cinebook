#ifndef MOVIE_SERVICE_HPP
#define MOVIE_SERVICE_HPP

#include "database.hpp"
#include "json.hpp"
#include <string>

namespace cinebook {

class MovieService {
private:
    IDatabase& db_;

public:
    explicit MovieService(IDatabase& db) : db_(db) {}

    nlohmann::json getMovies(
        const std::string& search = "",
        const std::string& genre = "",
        const std::string& language = "",
        int isUpcoming = -1
    );

    nlohmann::json getMovieById(int movieId);

    nlohmann::json getGenres();

    nlohmann::json getLanguages();

    nlohmann::json getShows(int movieId = 0, const std::string& date = "");

    nlohmann::json getShowDetailsAndSeats(int showId);
};

} // namespace cinebook

#endif // MOVIE_SERVICE_HPP
