/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marieke <marieke@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/28 15:09:15 by marieke           #+#    #+#             */
/*   Updated: 2025/03/28 15:10:52 by marieke          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CgiTest.hpp"

using namespace std;

int main(void)
{
	CgiTest	test;

	cout << test.executeCGI() << endl;
}