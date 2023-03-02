#include <algorithm>
#include <ctime>
#include <string>
#include <vector>

class Minesweeper {
public:
    using RenderedField = std::vector<std::string>;

    struct Cell {
        size_t x = 0;
        size_t y = 0;
    };

    enum class CellContent {
        EMPTY,
        MINE,
    };

    struct CellInfo {
        bool opened = false;
        bool flaged = false;
        CellContent content = CellContent::EMPTY;
        int64_t number_of_mines_in_near_cells = 0;
    };

    enum class GameStatus {
        NOT_STARTED,
        IN_PROGRESS,
        VICTORY,
        DEFEAT,
    };

    Minesweeper(size_t width, size_t height, size_t mines_count) : mines_(mines_count), titles_(width * height) {
        if (mines_count >= width * height) {
            field_ = std::vector<std::vector<CellInfo>>(
                height, std::vector<CellInfo>(width, CellInfo({false, false, CellContent::MINE, 0})));
        } else {
            field_ = std::vector<std::vector<CellInfo>>(height, std::vector<CellInfo>(width));
            if (mines_count > 0) {
                FillField();
            }
        }
    }

    Minesweeper(size_t width, size_t height, const std::vector<Cell>& cells_with_mines)
        : field_(height, std::vector<CellInfo>(width)), mines_(cells_with_mines.size()), titles_(width * height) {
        for (const auto& cell : cells_with_mines) {
            field_[cell.y][cell.x].content = CellContent::MINE;
        }
    }

    void NewGame(size_t width, size_t height, size_t mines_count) {
        Restart(mines_count, width * height);

        if (mines_count >= width * height) {
            field_ = std::vector<std::vector<CellInfo>>(
                height, std::vector<CellInfo>(width, CellInfo({false, false, CellContent::MINE, 0})));
        } else {
            field_ = std::vector<std::vector<CellInfo>>(height, std::vector<CellInfo>(width));
            if (mines_count > 0) {
                FillField();
            }
        }
    }

    void NewGame(size_t width, size_t height, const std::vector<Cell>& cells_with_mines) {
        field_ = std::vector<std::vector<CellInfo>>(height, std::vector<CellInfo>(width));
        Restart(cells_with_mines.size(), width * height);

        for (const auto& cell : cells_with_mines) {
            field_[cell.y][cell.x].content = CellContent::MINE;
        }
    }

    void OpenCell(const Cell& cell) {
        if (game_status_ == GameStatus::NOT_STARTED) {
            game_status_ = GameStatus::IN_PROGRESS;
            time(&game_time_);
        }

        if (game_status_ == GameStatus::IN_PROGRESS) {
            if (field_[cell.y][cell.x].content == CellContent::MINE) {
                for (auto& line : field_) {
                    OpenLine(line);
                }

                game_status_ = GameStatus::DEFEAT;
                game_time_ = static_cast<time_t>(difftime(time(nullptr), game_time_));
            } else {
                if (field_[cell.y][cell.x].opened) {
                    return;
                }

                field_[cell.y][cell.x].flaged = false;
                std::vector<Cell> buff;
                field_[cell.y][cell.x].opened = true;
                ++opened_cell_number_;
                Infect(buff, cell);

                if (mines_ + opened_cell_number_ == titles_) {
                    game_status_ = GameStatus::VICTORY;
                    game_time_ = static_cast<time_t>(difftime(std::time(nullptr), game_time_));
                }
            }
        }
    }

    void MarkCell(const Cell& cell) {
        if (game_status_ == GameStatus::NOT_STARTED) {
            game_status_ = GameStatus::IN_PROGRESS;
            time(&game_time_);
        }

        if (game_status_ == GameStatus::IN_PROGRESS && field_[cell.y][cell.x].opened == false) {
            field_[cell.y][cell.x].flaged = !field_[cell.y][cell.x].flaged;
        }
    }

    GameStatus GetGameStatus() const {
        return game_status_;
    }

    time_t GetGameTime() const {
        switch (game_status_) {
            case Minesweeper::GameStatus::NOT_STARTED:
                return static_cast<time_t>(0);
            case Minesweeper::GameStatus::IN_PROGRESS:
                return static_cast<time_t>(difftime(std::time(nullptr), game_time_));
            case Minesweeper::GameStatus::VICTORY:
                return game_time_;
            case Minesweeper::GameStatus::DEFEAT:
                return game_time_;
        }
    }

    RenderedField RenderField() const {
        std::vector<std::string> snapshot;

        for (const auto& line : field_) {
            std::string temp;

            for (const auto& cell : line) {
                if (cell.flaged == true) {
                    temp += '?';
                    continue;
                }

                if (!cell.opened) {
                    temp += '-';
                    continue;
                }

                if (cell.content == CellContent::MINE) {
                    temp += '*';
                    continue;
                }

                if (cell.number_of_mines_in_near_cells == 0) {
                    temp += '.';
                    continue;
                }

                temp += std::to_string(cell.number_of_mines_in_near_cells);
            }

            snapshot.emplace_back(temp);
        }

        return snapshot;
    }

private:
    CellInfo MineGenerator() {
        CellInfo temp;

        if (mine_counter_ == mines_) {
            return temp;
        }

        if (rand() % 2) {
            temp.content = CellContent::MINE;
            ++mine_counter_;
        }

        return temp;
    }

    void FillLine(std::vector<CellInfo>& line) {
        for (auto& cell : line) {
            cell = MineGenerator();
        }
    }

    void OpenLine(std::vector<CellInfo>& line) {
        for (auto& cell : line) {
            cell.opened = true;
        }
    }

    void FillField() {
        for (auto& line : field_) {
            FillLine(line);
        }
    }

    void Infect(std::vector<Cell>& cluster, const Cell& cell) {
        cluster.emplace_back(cell);
        int64_t number_of_mines = 0;
        bool infect_further = true;
        std::vector<Cell> next_cells;

        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                if (!((i != 0 || j != 0) && cell.y + i < field_.size() && cell.x + j < field_[0].size() &&
                      cell.x + j >= 0 && cell.y + i >= 0)) {
                    continue;
                }

                if (field_[cell.y + i][cell.x + j].content == CellContent::MINE) {
                    infect_further = false;
                    ++number_of_mines;
                    continue;
                }

                if (infect_further && !field_[cell.y + i][cell.x + j].opened &&
                    !field_[cell.y + i][cell.x + j].flaged) {
                    next_cells.push_back({cell.x + j, cell.y + i});
                }
            }
        }

        field_[cell.y][cell.x].number_of_mines_in_near_cells = number_of_mines;

        if (infect_further) {
            for (const auto& next_cell : next_cells) {
                field_[next_cell.y][next_cell.x].opened = true;
                ++opened_cell_number_;
            }

            for (const auto& next_cell : next_cells) {
                Infect(cluster, next_cell);
            }
        }
    }

    void Restart(const size_t& mines_count, const size_t& title_count) {
        game_status_ = GameStatus::NOT_STARTED;
        mine_counter_ = 0;
        opened_cell_number_ = 0;
        mines_ = mines_count;
        titles_ = title_count;
    }

    GameStatus game_status_ = GameStatus::NOT_STARTED;
    std::vector<std::vector<CellInfo>> field_;
    time_t game_time_;
    size_t opened_cell_number_ = 0;
    size_t mine_counter_ = 0;
    size_t mines_ = 0;
    size_t titles_ = 0;
};