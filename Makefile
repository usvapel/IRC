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

# Terminal colors for build output
BOLD			:= $(shell tput bold)
GREEN			:= $(shell tput setaf 2)
YELLOW			:= $(shell tput setaf 3)
BLUE			:= $(shell tput setaf 4)
MAGENTA			:= $(shell tput setaf 5)
CYAN			:= $(shell tput setaf 6)
WHITE			:= $(shell tput setaf 7)
RESET			:= $(shell tput sgr0)

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ SOURCE FILES ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

SRCS_MAIN := \
	main.cpp \

# Combine all source files
SRCS := \
	$(SRCS_MAIN)

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ BUILD VARIABLES ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ #

# Derived build variables
OBJS				:= $(addprefix $(OBJ_DIR)/,$(SRCS:.cpp=.o))
TOTAL_SRCS			:= $(words $(SRCS))

SHELL := /bin/bash

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

# Default target
all: _reset_progress $(NAME) 
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


LOCK_FILE     := $(OBJ_DIR)/.build.lock

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

# ■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■ TARGET DECLARATIONS ■■■■■■■■■■■■■■■■■■■■■■■■■ #
.PHONY: all debug clean fclean re
