NAME = webserv
CPP = c++
CPPFLAGS_BASE = -Wall -Werror -Wextra -std=c++17 -MMD -fPIE
DEBUGFLAGS = -g3
LDFLAGS = -lstdc++fs
RM = rm -rf

SRC_DIR = src
OBJ_DIR_BASE = obj
OBJ_DIR = $(OBJ_DIR_BASE)
ifeq ($(BUILD_TYPE),debug)
	CPPFLAGS := $(CPPFLAGS_BASE) $(DEBUGFLAGS)
	OBJ_DIR := $(OBJ_DIR_BASE)_debug
else
	CPPFLAGS := $(CPPFLAGS_BASE)
endif

SRCS = $(wildcard $(SRC_DIR)/*.cpp) \
       $(wildcard $(SRC_DIR)/Networks/*.cpp) \
       $(wildcard $(SRC_DIR)/Parsing/*.cpp) \
       $(wildcard $(SRC_DIR)/Server/*.cpp) \
       $(wildcard $(SRC_DIR)/CGI/*.cpp)

OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

all: $(OBJ_DIR) $(NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/Networks
	mkdir -p $(OBJ_DIR)/Parsing
	mkdir -p $(OBJ_DIR)/Server
	mkdir -p $(OBJ_DIR)/CGI

$(NAME): $(OBJS)
	$(CPP) $(CPPFLAGS) $(OBJS) -o $(NAME) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CPP) $(CPPFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	$(RM) obj obj_debug

fclean: clean
	$(RM) $(NAME)

re: fclean all

debug:
	$(MAKE) BUILD_TYPE=debug

.PHONY: all clean fclean re debug
