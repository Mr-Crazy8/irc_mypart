CPP    = c++
NAME   = ircserv
FLAGS  = -Wall -Wextra -Werror -std=c++98 -MMD

SRC    = main.cpp \
         Server.cpp \
         Client.cpp \
         Channel.cpp \
         Message.cpp \
         Commands.cpp

OBJ    = $(SRC:.cpp=.o)
DEPS   = $(OBJ:.o=.d)

$(NAME): $(OBJ)
	$(CPP) $(FLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CPP) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(DEPS)

fclean: clean
	rm -f $(NAME)

re: fclean $(NAME)

-include $(DEPS)
