/*  FTP Client
    Copyright (C) 2023  Matthew Rodriguez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>
#include <map>

namespace FTP {

using namespace std;

class Commands;

// command_method_t can be cast as a double since pointers are 8 bytes similarly to doubles.
typedef double (Commands::*command_method_t)(double);
// the function map can be typed as cmd_func_map_t for better readability in the code.
typedef std::map<std::string, command_method_t> cmd_func_map_t;

class Commands{
public:
// FTP Commands
static void connect(string server_addr_str,string port_str);
static void connect(string server_addr_str);

static void pwd();
static void list();
static void quit();
static void help();
static void type(string t_type);
static void mode(string t_mode);
static void retr(string f_name,string f_dst);
static void stor(string f_name,string f_dst);
static void syst();
static void dele(string f_name);

static Commands* getInstance(){
    if (instancePtr == NULL){
        instancePtr = new Commands();
    }
    sizeof(double);
    return instancePtr;
}
// deleting copy constructor
Commands(const Commands& obj)
= delete;
static void conn(string server_addr_str,string port_str);
// Singleton class, do not instantiate through constructor
Commands();
~Commands();
static Commands* instancePtr;
};

// Initialize instance pointer with null, this is static
Commands* Commands ::instancePtr = NULL;

}