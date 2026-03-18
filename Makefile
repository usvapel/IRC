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
OBJ_DIR			:= obj
DEP_DIR			:= $(OBJ_DIR)/.deps

# Tell Make to look in $(SRC_DIR)
# when resolving dependencies for %.o targets
VPATH			:= $(SRC_DIR)

# Include paths
INC				:= -I./include

# Dependency generation flags
DEPFLAGS		= -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.d
MAKEFLAGS		+= -s

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ VISUAL STYLING ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

COLORS := $(shell tput colors 2>/dev/null || echo 0)
ifeq ($(shell [ $(COLORS) -ge 256 ] && echo yes),yes)
GIT_HASH	:= $(shell tput setaf 39)
GIT_AUTHOR	:= $(shell tput setaf 213)
GIT_BRANCH	:= $(shell tput setaf 81)
GIT_TAG		:= $(shell tput setaf 220)
GIT_HEADER	:= $(shell tput setaf 45)
GIT_LABEL	:= $(shell tput setaf 250)
else
GIT_HASH	:= $(shell tput setaf 4)
GIT_AUTHOR	:= $(shell tput setaf 1)
GIT_BRANCH	:= $(shell tput setaf 6)
GIT_TAG		:= $(shell tput setaf 3)
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

SRCS_MAIN	:= \
	main.cpp \
	Logger.cpp \

# Combine all source files
SRCS		:= \
	$(SRCS_MAIN)

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ BUILD VARIABLES ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

# Derived build variables
OBJS			:= $(addprefix $(OBJ_DIR)/,$(SRCS:.cpp=.o))
TOTAL_SRCS		:= $(words $(SRCS))
LOCK_FILE		:= $(OBJ_DIR)/.build.lock
GIT_COMMIT		:= $(shell git rev-parse --short HEAD)
GIT_AUTHOR_NAME	:= $(shell git show --format="%an <%ae>" -s $(GIT_COMMIT))
GIT_BRANCH		:= $(shell git rev-parse --abbrev-ref HEAD)
GIT_TAG_NAME 	:= $(shell git rev-parse --abbrev-ref --tags)

SHELL	:= /bin/bash

# Displays an animated progress bar with spinner, percentage,
# and current file name during build
define PROGRESS
	IDX=$$(( $$(cat $(LOCK_FILE) 2>/dev/null || echo 0) + 1 )); \
	echo $$IDX > $(LOCK_FILE); \
	PCT=$$((IDX * 100 / $(TOTAL_SRCS))); \
	BAR_FILLED=$$((IDX * 20 / $(TOTAL_SRCS))); \
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
			0) SPIN="⠋";; 1) SPIN="⠙";; 2) SPIN="⠹";; 3) SPIN="⠸";; 4) SPIN="⠼";; \
			5) SPIN="⠴";; 6) SPIN="⠦";; 7) SPIN="⠧";; 8) SPIN="⠇";; 9) SPIN="⠏";; \
		esac; \
	fi; \
	printf "\r$$SPIN $$COLOR$$FILLED$$EDGE$$EMPTY\033[0m %3d%% %-30s" $$PCT "$<"
endef

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ BUILD TARGETS ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

.PHONY: all debug clean fclean re
# Default target
all: _reset_progress print-version $(NAME) 
	@if [ ! -f $(OBJ_DIR)/.built ]; then \
		echo ">$(BOLD)$(YELLOW) $(NAME) is already up to date.$(RESET)"; \
	else \
		rm -f $(OBJ_DIR)/.built; \
		echo ">$(BOLD)$(GREEN) All components built successfully!$(RESET)"; \
	fi

# Main executable linking with dependency checking
$(NAME): $(OBJS)
	@printf "\n"
	@echo ">$(BOLD)$(GREEN) Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	@touch $(OBJ_DIR)/.built
	@echo ">$(BOLD)$(GREEN) $(NAME) successfully linked!$(RESET)"

.PHONY: _reset_progress
_reset_progress:
	@rm -f $(LOCK_FILE)

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR) $(DEP_DIR)
	@( flock 9; $(PROGRESS) ) 9<>$(LOCK_FILE)
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@ $(INC)

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ ADDITIONAL TARGETS ■■■■■■■■■■■■■■■■■■■■■■■■■■ #

# Directory creation and dependency management
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Include auto-generated dependency files
-include $(wildcard $(DEP_DIR)/*.d)

$(DEP_DIR): | $(OBJ_DIR)
	@mkdir -p $@

print-version:
	@echo "$(BOLD)$(GIT_HEADER)━━ Git Build Info ━━$(RESET)"
	@echo "$(GIT_LABEL)Branch:$(RESET)  $(GIT_BRANCH)$(GIT_BRANCH_NAME)$(RESET)"
	@echo "$(GIT_LABEL)Author:$(RESET)  $(GIT_AUTHOR)$(GIT_AUTHOR_NAME)$(RESET)"
	@echo "$(GIT_LABEL)Commit:$(RESET)  $(GIT_HASH)$(GIT_COMMIT)$(RESET)"
	@echo "$(GIT_LABEL)Tag:$(RESET)     $(GIT_TAG)$(GIT_TAG_NAME)$(RESET)"

# Development and debugging build configuration
debug: CXXFLAGS	+= $(DEBUG_FLAGS)
debug: clean $(NAME)
	@echo ">$(BOLD)$(CYAN)  Debug build completed!$(RESET)"

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
	@if [ -f $(NAME) ]; then \
		echo "> [ $(NAME) ] $(YELLOW) Removing $(NAME)...$(RESET)"; \
		rm -f $(NAME); \
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
