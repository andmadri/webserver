/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marieke <marieke@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/24 16:08:20 by maraasve          #+#    #+#             */
/*   Updated: 2025/03/25 15:12:41 by marieke          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiTest.hpp"

int	main(void)
{
	CgiTest	test;
	std::string	output;

	output = test.executeCGI();
	std::cout << output << std::endl;
	std::cout << output << std::endl;
}