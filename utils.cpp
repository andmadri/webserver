/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: maraasve <maraasve@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/28 15:36:56 by marieke           #+#    #+#             */
/*   Updated: 2025/03/31 13:34:03 by maraasve         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils.hpp"

void	freeArray(char **array) //put this in free.cpp or smh
{
	size_t	i = 0;

	while (array[i])
	{
		free(array[i]);
		i++;
	}
	free(array);
}

char	**vecToArray(std::vector<std::string> vec)
{
	size_t	size;
	char	**array;
	
	size = vec.size();
	array = static_cast<char **>(calloc(size + 1, sizeof(char *)));
	if (!array)
		return (nullptr);
	for (size_t i = 0;i < size; i++)
	{
		array[i] = strdup(vec[i].c_str());
		if (!array[i])
		{
			freeArray(array);
			return (nullptr);
		}
	}
	return (array);
}

std::vector<std::string>	vecSplit(char *str, char delim)
{
	std::vector<std::string>	vec;
	std::istringstream			stream(str);
	std::string					segment;

	while (getline(stream, segment, delim))
		vec.push_back(segment);
	return (vec);
}