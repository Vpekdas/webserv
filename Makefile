# ============================== PROJECT INFO ================================ #
NAME			:= webserv
PROJECT_NAME	:= webserv

# =========================== COMPILER AND FLAGS ============================= #
CXX				:= clang++
CXXFLAGS		:= -std=c++98 -Wall -Wextra -Werror -g3 -O2 -Isrc -D_DEBUG
DEPFLAGS		:= -MMD -MP

# ================================= ALIASES ================================== #
SRCS_PATH = src/
OBJS_PATH = obj/

RM = rm -rf

# =============================== ANSI CODES ================================= #

# utils
ERASE_L			:= \033[K
CURS_UP			:= \033[A
SAVE_CURS_POS	:= \033[s
LOAD_CURS_SAVE	:= \033[u
BOLD			:= \033[1m
BLINK			:= \033[5m

# reset
NC				:= \033[0m

# colors
YELLOW			:= \033[0;33m
GREEN			:= \033[0;32m
BLUE			:= \033[0;34m
RED				:= \033[0;31m
PURPLE			:= \033[0;35m
CYAN			:= \033[0;36m
BLACK			:= \033[0;30
WHITE			:= \033[0;37m

# bold + colors
BYELLOW			:= \033[1;33m
BGREEN			:= \033[1;32m
BBLUE			:= \033[1;34m
BRED			:= \033[1;31m
BPURPLE			:= \033[1;35m
BCYAN			:= \033[1;36m
BBLACK			:= \033[1;30m
BWHITE			:= \033[1;37m

# advanced colors
A_BLACK			:= \033[38;5;232m

# bg colors
GREEN_BG		:= \033[48;5;2m
WHITE_BG		:= \033[48;5;15m

# ================================ SRC FILES ================================= #

SRCS 			:=	$(addprefix $(SRCS_PATH), \
					main.cpp \
					webserv.cpp \
					connection.cpp \
					server.cpp \
					file.cpp \
					router.cpp \
					logger.cpp \
					cgi/cgi.cpp \
					config/config.cpp \
					config/parser.cpp \
					http/request.cpp \
					http/response.cpp \
					http/status.cpp \
)

# ================================ OBJ FILES ================================= #

OBJS 			:=	$(patsubst $(SRCS_PATH)%.cpp,$(OBJS_PATH)%.o,$(SRCS))

# ================================ DEPS FILES ================================= #

DEPS			:=	$(patsubst $(SRCS_PATH)%.cpp,$(OBJS_PATH)%.d,$(SRCS))

# ============================= FORMATTING VARS ============================== #

# counting files vars
TOTAL			:= $(words $(SRCS))
FILE_COUNT		:= 0

# progress bar vars
BAR_COUNT		:= 0
BAR_PROGRESS	:= 0
BAR_SIZE		:= 64

# gradient G vars
GRAD_G_PROG		:= 0
GRAD_G_SIZE		:= 12
GRADIENT_G		:= \033[38;5;160m \
				\033[38;5;196m \
				\033[38;5;202m \
				\033[38;5;208m \
				\033[38;5;214m \
				\033[38;5;220m \
				\033[38;5;226m \
				\033[38;5;190m \
				\033[38;5;154m \
				\033[38;5;118m \
				\033[38;5;82m \
				\033[38;5;46m

# gradient B vars
GRAD_B_PROG		:= 0
GRAD_G_SIZE		:= 0
GRADIENT_B		:= \033[38;5;2m \
				 \033[38;5; \
				 \033[38;5; \
				 \033[38;5; \
				 \033[38;5; \
				 \033[38;5; \
				 \033[38;5; \
				 \033[38;5; \
				 \033[38;5; \
				 \033[38;5;

# function to get the current color of the bar by index in the gradient
define GET_G_GRADIENT
$(word $(1),$(GRADIENT_G))
endef

all: $(NAME)

portfolio:
	git clone https://github.com/Vpekdas/Vpekdas.github.io sites/portfolio && cd sites/portfolio && npm install && npm run build

extra: portfolio

$(NAME): $(OBJS)
#	==================== draw progress bar ===================
#	=========== erase prev line + write "compiling" ==========
	@printf "\t"
	@for N in $$(seq 1 $(shell echo $$(($(BAR_SIZE) + 2)))); do \
		echo -n █; \
	done
	@printf "\r"
	@echo "$(WHITE_BG)$(A_BLACK)$(BOLD)\t Compiling:$(NC)"
#	=============== draw finished progress bar ===============
	@printf "\t█$(GREEN)"
	@for N in $$(seq 1 $(BAR_PROGRESS)); do \
		echo -n █; \
	done
#	============= save position of cursor (eol) ==============
	@printf "$(SAVE_CURS_POS)"
#	======== go back to the middle of the line with \b =======
	@$(eval BAR_PROGRESS=$(shell echo $$(($(BAR_PROGRESS) / 2))))
	@for N in $$(seq 1 $(BAR_PROGRESS)); do \
		echo -n "\b"; \
	done
#	====================== print "COMPLETE" ======================
	@printf "\b\b\b\b$(NC)$(BLINK)$(BOLD)$(GREEN_BG)COMPLETE"
#	= go back to the saved position (eol) and go up one line =
	@printf "$(LOAD_CURS_SAVE)$(NC)█$(CURS_UP)"
#	==== go back several characters and print percentage =====
	@printf "\b\b\b\b\b$(A_BLACK)$(WHITE_BG)$(BOLD)%3d%%$(NC)\r" $(PERCENT)
#	================= write rest of messages =================
	@echo "\n\n\n[🔘] $(BGREEN)$(PROJECT_NAME) Ready !$(NC)\n"
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	@printf "[✨] $(BCYAN)[%2d/%2d]\t$(BWHITE)All files have been compiled ✔️$(NC)\n" $(FILE_COUNT) $(TOTAL)

-include $(DEPS)

$(OBJS_PATH)%.o: $(SRCS_PATH)%.cpp
	@mkdir -p $(OBJS_PATH)
	@mkdir -p $(OBJS_PATH)/http
	@mkdir -p $(OBJS_PATH)/cgi
	@mkdir -p $(OBJS_PATH)/config
#	TODO: @mkdir -p $(OBJS_PATH)/sub directory name
	@$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@
	@$(eval FILE_COUNT=$(shell echo $$(($(FILE_COUNT)+1))))
	@$(eval PERCENT:=$(shell echo $$((100*$(FILE_COUNT)/$(TOTAL)))))
#	================= calculate progress bar =================
	@$(eval BAR_PROGRESS=$(shell echo $$(($(BAR_SIZE)*$(FILE_COUNT)/$(TOTAL)))))
#	================== calculate bar color ===================
	@$(eval GRAD_G_PROG=$(shell echo $$(($(GRAD_G_SIZE)*$(FILE_COUNT)/$(TOTAL) + 1))))
#	========== printing compiling file + percentage ==========
	@printf "\t"
	@for N in $$(seq 1 $(shell echo $$(($(BAR_SIZE) + 2)))); do \
		echo -n █; \
	done
	@printf "\r"
	@printf "\t$(A_BLACK)$(BOLD)$(WHITE_BG) Compiling: $@%*s...$(NC)\n"
#	=================== draw progress bar ====================
	@printf "\t█$(call GET_G_GRADIENT, $(GRAD_G_PROG))"
	@for N in $$(seq 1 $(BAR_PROGRESS)); do \
		echo -n █; \
	done
	@for N in $$(seq 1 $(shell echo $$(($(BAR_SIZE) - $(BAR_PROGRESS))))); do \
		echo -n ░; \
	done
	@printf "$(NC)█\n\t"
	@for N in $$(seq 1 $(shell echo $$(($(BAR_SIZE) + 2)))); do \
		echo -n ▀; \
	done
	@printf "$(CURS_UP)$(CURS_UP)"
	@printf "\b\b\b\b\b$(A_BLACK)$(WHITE_BG)$(BOLD)%3d%%$(NC)\r" $(PERCENT)
#	==========================================================

clean:
	@printf "$(PURPLE)"
	@$(RM) $(OBJS) $(OBJS_PATH)
	@echo "[🧼] $(BYELLOW)Objects $(YELLOW)files have been cleaned from $(PROJECT_NAME) ✔️$(NC)\n"

fclean: clean
	@printf "$(PURPLE)"
	@$(RM) $(NAME)
	@echo "[🚮] $(BRED)All $(RED)files have been cleaned ✔️$(NC)\n"

re: clean all

.PHONY: all clean fclean re
