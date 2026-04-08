# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ SETTINGS ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

# Project configuration
NAME			:= irc
CXX				:= c++

# Compiler flags
CXXFLAGS		:= -Wall -Wextra -Werror -Wshadow -std=c++20
CXXFLAGS		+= -O2
DEBUG_FLAGS		:= -g3 -fsanitize=address -fsanitize=undefined -O0

# Directory structure
SRC_DIR			:= src
TEST_DIR        := tests
OBJ_DIR			:= obj
DEP_DIR			:= $(OBJ_DIR)/.deps

# Tell Make to look in $(SRC_DIR)
# when resolving dependencies for %.o targets
VPATH			:= $(SRC_DIR):$(TEST_DIR)

# Include paths
INC				:= -I./include

# Dependency generation flags
DEPFLAGS		= -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.d
MAKEFLAGS		+= -s

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ VISUAL STYLING ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

COLORS				:= $(shell tput colors 2>/dev/null || echo 0)
ifeq ($(shell [ $(COLORS) -ge 256 ] && echo yes),yes)
GIT_HASH_COLOR		:= $(shell tput setaf 73)
GIT_AUTHOR_COLOR	:= $(shell tput setaf 153)
GIT_BRANCH_COLOR	:= $(shell tput setaf 150)
GIT_TAG_COLOR		:= $(shell tput setaf 179)
GIT_HEADER_COLOR	:= $(shell tput setaf 67)
GIT_LABEL_COLOR		:= $(shell tput setaf 246)
else
GIT_HASH			:= $(shell tput setaf 4)
GIT_AUTHOR			:= $(shell tput setaf 5)
GIT_BRANCH			:= $(shell tput setaf 6)
GIT_TAG				:= $(shell tput setaf 3)
endif

# Terminal colors for build output
BOLD		:= $(shell tput bold)
RED			:= $(shell tput setaf 1)
GREEN		:= $(shell tput setaf 2)
YELLOW		:= $(shell tput setaf 3)
BLUE		:= $(shell tput setaf 4)
MAGENTA		:= $(shell tput setaf 5)
CYAN		:= $(shell tput setaf 6)
WHITE		:= $(shell tput setaf 7)
RESET		:= $(shell tput sgr0)

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ SOURCE FILES ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

SRCS_CORE	:= \
	Logger.cpp \
	Socket.cpp \
	Parser.cpp \
	Client.cpp \
	Utils.cpp \
	Channel.cpp \
	ServerCore.cpp \
	ServerHandshake.cpp \
	ServerReplies.cpp \
	ServerHandlers.cpp \

# Combine all source files
SRCS		:= \
	$(SRCS_CORE) \
	main.cpp \

# Source files needed for testing
TEST_SRCS := \
   channel_test.cpp  \

TEST_EXEC := test_runner

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ BUILD VARIABLES ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

# Derived build variables
OBJS				:= $(addprefix $(OBJ_DIR)/,$(SRCS:.cpp=.o))
CORE_OBJS           := $(addprefix $(OBJ_DIR)/,$(SRCS_CORE:.cpp=.o))
TEST_OBJS           := $(addprefix $(OBJ_DIR)/,$(TEST_SRCS:.cpp=.o))
TOTAL_SRCS			:= $(words $(SRCS))
LOCK_FILE			:= $(OBJ_DIR)/.build.lock

# git log variables
GIT_HASH			:= $(shell git rev-parse --short HEAD)
GIT_AUTHOR			:= $(shell git show --format="%an <%ae>" \
						-s $(GIT_HASH))
GIT_BRANCH			:= $(shell git rev-parse --abbrev-ref HEAD)
GIT_TAG 			:= $(shell git rev-parse --abbrev-ref --tags)
GIT_REMOTE_STATUS	:= $(shell git rev-list --left-right --count \
						origin/$(GIT_BRANCH)...HEAD 2>/dev/null | \
						awk '{print "↓"$$1" ↑"$$2}')
GIT_COMMIT_COUNT	:= $(shell git rev-list --count HEAD)
GIT_DATE        	:= $(shell git show -s --format="%cd" \
						--date=format:"%Y-%m-%d %H:%M" HEAD)

# Injects a fingerprint macro at build time, so program can log current build
CXXFLAGS += -DGIT_HASH=\"$(GIT_HASH)\"

SHELL	:= /bin/bash

# Adds a buld fingerprint as a macro available in
CXXFLAGS += -DGIT_HASH=\"$(GIT_HASH)\"

# Displays an animated progress bar with spinner, percentage,
# and current file name during build
define PROGRESS
	IDX=$$(( $$(cat $(LOCK_FILE) 2>/dev/null || echo 0) + 1 )); \
	echo $$IDX > $(LOCK_FILE); \
	PCT=$$((IDX * 100 / $(TOTAL_SRCS))); \
	PCT=$$((PCT > 100 ? 100 : PCT)); \
	BAR_FILLED=$$((IDX * 20 / $(TOTAL_SRCS))); \
	BAR_FILLED=$$((BAR_FILLED > 20 ? 20 : BAR_FILLED)); \
	BAR_EMPTY=$$((20 - BAR_FILLED)); \
	FILLED_BODY=$$((BAR_FILLED > 1 ? BAR_FILLED - 1 : 0)); \
	FILLED=""; for ((i=0; i<FILLED_BODY; i++)); do FILLED+="━"; done; \
	EMPTY="";  for ((i=0; i<BAR_EMPTY;  i++)); do EMPTY+="─";  done; \
	if   [ $$BAR_FILLED -eq 20 ]; then EDGE="━"; \
	elif [ $$BAR_FILLED -gt 0 ];  then EDGE="╸"; \
	else EDGE=""; fi; \
	if   [ $$PCT -lt 40 ]; then COLOR="\033[38;5;22m"; \
	elif [ $$PCT -lt 70 ]; then COLOR="\033[38;5;34m"; \
	elif [ $$PCT -lt 90 ]; then COLOR="\033[38;5;40m"; \
	else                        COLOR="\033[38;5;46m"; fi; \
	if [ $$PCT -eq 100 ]; then SPIN="✓"; \
	else \
		case $$((IDX % 10)) in \
			0) SPIN="⠋";; \
			1) SPIN="⠙";; \
			2) SPIN="⠹";; \
			3) SPIN="⠸";; \
			4) SPIN="⠼";; \
			5) SPIN="⠴";; \
			6) SPIN="⠦";; \
			7) SPIN="⠧";; \
			8) SPIN="⠇";; \
			9) SPIN="⠏";; \
		esac; \
	fi; \
	printf "\r$$SPIN $$COLOR$$FILLED$$EDGE$$EMPTY\033[0m %3d%% %-30s" $$PCT "$<"
endef

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ BUILD TARGETS ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

.PHONY: all debug clean fclean re print-version
# Default target
all: print-version $(NAME)
	@if [ ! -f $(OBJ_DIR)/.built ]; then \
		echo ">$(BOLD)$(YELLOW) $(NAME) is already up to date.$(RESET)"; \
	else \
		rm -f $(OBJ_DIR)/.built; \
		echo ">$(BOLD)$(GREEN) All components built successfully!$(RESET)"; \
	fi

$(NAME): $(OBJS)
	@printf "\n"
	@echo ">$(BOLD)$(GREEN) Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	@touch $(OBJ_DIR)/.built
	@echo ">$(BOLD)$(GREEN) $(NAME) successfully linked!$(RESET)"

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR) $(DEP_DIR)
	@( flock 9; $(PROGRESS) ) 9<>$(LOCK_FILE)
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@ $(INC)

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ ADDITIONAL TARGETS ■■■■■■■■■■■■■■■■■■■■■■■■■■ #

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

-include $(wildcard $(DEP_DIR)/*.d)

$(DEP_DIR): | $(OBJ_DIR)
	@mkdir -p $@

# git logs
print-version:
	@echo "$(BOLD)$(GIT_HEADER_COLOR)━━ Git Build Info ━━$(RESET)"
	@echo "$(GIT_LABEL_COLOR)Branch:$(RESET)  $(GIT_BRANCH_COLOR)$(GIT_BRANCH)   $(GIT_REMOTE_STATUS)$(RESET)"
	@echo "$(GIT_LABEL_COLOR)Author:$(RESET)  $(GIT_AUTHOR_COLOR)$(GIT_AUTHOR)$(RESET)"
	@echo "$(GIT_LABEL_COLOR)Commit:$(RESET)  $(GIT_HASH_COLOR)$(GIT_HASH)  $(GIT_DATE)  ($(GIT_COMMIT_COUNT) commits)$(RESET)"
	@echo "$(GIT_LABEL_COLOR)Tag:$(RESET)     $(GIT_TAG_COLOR)$(GIT_TAG)$(RESET)"

debug: CXXFLAGS	+= $(DEBUG_FLAGS)
debug: clean $(NAME)
	@echo ">$(BOLD)$(CYAN)  Debug build completed!$(RESET)"

$(TEST_EXEC): $(CORE_OBJS) $(TEST_OBJS)
	@printf "\n"
	@echo ">$(BOLD)$(GREEN) Linking $(TEST_EXEC)...$(RESET)"
	@$(CXX) $(CXXFLAGS) -o $(TEST_EXEC) $(CORE_OBJS) $(TEST_OBJS) $(INC)
	@echo ">$(BOLD)$(GREEN) $(TEST_EXEC) successfully linked!$(RESET)"

test: $(TEST_EXEC)

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ CLEAN TARGETS ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

PADDING = $(shell echo -n "$(NAME)" | wc -c)

# Remove object files and build artifacts
clean:
	@if [ -d $(OBJ_DIR) ]; then \
		echo "> [ $(NAME) ] $(YELLOW) Cleaning object files...$(RESET)"; \
		rm -rf $(OBJ_DIR); \
		printf "> [ %-$(PADDING)s ] %b %s Object files cleaned!%b\n" "$(NAME)" \
		"$(YELLOW)" "$(NAME)" "$(RESET)"; \
	else \
		echo "> [ $(NAME) ] $(BOLD)$(YELLOW) Nothing to be done with \
		$(RESET)$(BOLD)$(WHITE)clean$(RESET)"; \
	fi

# Complete cleanup including executables and external libraries
fclean: clean
	@if [ -f $(NAME) ] || [ -f $(TEST_EXEC) ]; then \
		echo "> [ $(NAME) ] $(YELLOW) Removing $(NAME)...$(RESET)"; \
		rm -f $(NAME) $(TEST_EXEC); \
		printf "> [ %-$(PADDING)s ] %b %s removed!%b\n" "$(NAME)" "$(YELLOW)" \
		"$(NAME)" "$(RESET)"; \
	else \
		echo "> [ $(NAME) ] $(BOLD)$(YELLOW) Nothing to be done with \
		$(RESET)$(BOLD)$(WHITE)fclean$(RESET)"; \
	fi

# Full rebuild from clean slate
re:
	@echo "> [ $(NAME) ] $(BOLD)$(WHITE) Rebuilding from scratch...$(RESET)"
	@$(MAKE) fclean
	@$(MAKE) all
