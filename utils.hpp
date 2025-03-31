/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maraasve <maraasve@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/28 16:10:44 by marieke           #+#    #+#             */
/*   Updated: 2025/03/31 13:34:45 by maraasve         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

# include <iostream>
# include <vector>
# include <cstring>
# include <sstream>

char						**vecToArray(std::vector<std::string> vec);
std::vector<std::string>	vecSplit(char *str, char delim);
void						freeArray(char **array);

#endif