#include <vector>
#include <string>
#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <memory>
#include <array>
#include <fstream>
#include <format>
#include <regex>
#include <pybind11/embed.h> 
#include <chrono>
#include <cctype>
#include <boost/process.hpp>

namespace timer = std::chrono;

namespace py = pybind11;

int get_exit_code(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    std::string line;
    std::string last_line;
    while (std::getline(file, line)) {
        last_line = line;
    }

    file.close();

    std::regex exit_code_regex(R"(\[COMMAND_EXIT_CODE="(\d+)\"])");
    std::smatch match;
    if (std::regex_search(last_line, match, exit_code_regex) && match.size() > 1) {
        return std::stoi(match.str(1));
    } else {
        throw std::runtime_error("Exit code not found in the last line");
    }
}

std::string ansi_to_html(const std::string& ansi_text) {
    py::scoped_interpreter guard{};

    try {
        py::module sys = py::module::import("sys");

        std::string python_code = R"(
from ansi2html import Ansi2HTMLConverter

def convert_ansi_to_html(ansi_text):
    conv = Ansi2HTMLConverter()
    html_text = conv.convert(ansi_text)
    return html_text
)";

            py::exec(python_code.c_str());

        py::module main = py::module::import("__main__");
        py::object convert_ansi_to_html = main.attr("convert_ansi_to_html");

        py::object result = convert_ansi_to_html(ansi_text);

        return result.cast<std::string>();
    } catch (const py::error_already_set& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return "";
    }
} 

std::string extract_pre_content(const std::string& html) {
    std::vector<std::string> lines;
    std::istringstream stream(html);

    std::string output = "";

    bool pre_start = false, pre_end = false;

    int cnt = 0;

    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("pre ") != std::string::npos) {
            pre_start = true;
            output += line + "\n";
        }
        else if (line.find("</pre>") != std::string::npos) {
            pre_end = true;
            output += line;
        }
        else if (pre_start && !pre_end) {
            if (!std::all_of(line.begin(), line.end(), [](unsigned char ch) { return std::isspace(ch); })) 
                cnt++;
            output += line + "\n";
        }
    }

    if (cnt == 0) {
        return "";
    }
    return output;
}

std::string create_html(const std::string& output, std::string& commands) {
    std::string html = R"(<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title></title>
<style type="text/css">
.ansi2html-content { display: inline; white-space: pre-wrap; word-wrap: break-word; }
.body_foreground { color: #f5f3f3; }
.body_background { background-color: #ffffff; } /* Set background to white */
.inv_foreground { color: #000000; }
.inv_background { background-color: #faf9f9; }
.ansi1 { font-weight: bold; color: #0b0b0b; }
.ansi2 { font-weight: bold; color: #f7f7f7; }
.ansi32 { color: #00aa00; }
.ansi35 { color: #E850A8; }
.command-box {
   border: 2px solid #0332b2;
   background-color:  #0332b2;
   padding: 5px;
   display: inline-block;
   border-top-left-radius: 8px;
   border-top-right-radius: 8px;
   margin-bottom: 0; /* Remove bottom margin to touch the error box */
}
.forge-box {
    border: 2px solid #0e0e0e;
    padding: 5px;
    background-color:  #f6f6f7;
    margin-bottom: 0; /* Remove bottom margin to touch the error box */
    display: block;
    border-radius: 8px;
    text-align: center;
    margin: 0 auto;
    max-width: 400px;
}
.error-box {
    border: 2px solid #0332b2;
    padding: 10px;
    background-color: #fafafa;
    border-bottom-left-radius: 8px;
    border-bottom-right-radius: 8px;
    border-top-right-radius: 8px;
    color: #0b0a0a;
    white-space: pre-wrap;
    word-wrap: break-word;
}

table {
    width: 100%;
    border-collapse: collapse;
    overflow: hidden;
}

td, th {
    padding: 5px;
    vertical-align: middle;
    /* border: 1px solid #000; Inner borders - Black */
}

th {
    background-color: #f2f2f2;
    font-weight: bold;
    text-align: left;
    color: #000; /* Ensures text is black */
}

td:first-child {
    width: 20%; /* Adjust as needed */
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}

td:nth-child(2) {
    width: 80%;
}

tr:nth-child(even) {
    background-color: #f9f9f9; /* Alternating row colors for better readability */
}

tr:hover {
    background-color: #f1f1f1; /* Light hover effect */
}

.timer-table { width: 100%; border-collapse: collapse; }
.timer-table th, .timer-table td { border: 1px solid#0d0d0e; overflow: hidden; padding: 8px; color: black}
.timer-table th { background-color: #600780; text-align: left; color: white}
.timer-table td { vertical-align: middle; }
.timer-bar { display: inline-block; height: 20px; background-color: #4CAF50; position: relative; }
.timer-bar:hover::after { content: attr(data-time) " ms"; position: absolute; background-color: #fff; border: 1px solid #ddd; padding: 2px 5px; }
.command-cell { width: 150px; } /* Set a fixed width for the command column */
</style>
</head>
<body class="body_foreground body_background" style="font-size: normal;" >
<h1 style = "text-align:center; color: black" class = "forge-box"><a href = "https://github.com/Kronos-192081/forge">forge</a> build results</h1>
<h2 style = color:black;> Build Summary: </h2>
<table class="timer-table">
    <thead>
    <tr>
    <th style = "text-align: center;">Command</th>
    <th style = "text-align: center;">Time</th>
    </tr>
    </thead>
    <tbody id="timer-body">
    <!-- Rows will be generated dynamically -->
    </tbody>
</table>

<br>

<h2 style = color:black;> Compilation Details: </h2>)";

html += output;
 
html += R"(<script>
    const commands = [)";

html += commands;

html += R"(];
    
    let sum = 0;
    commands.forEach(element => {
        sum = sum + element.time;
    });
    const tableBody = document.getElementById('timer-body');

    let accumulatedTime = 0;
    
    commands.forEach((command, index) => {
        const row = document.createElement('tr');
        const commandCell = document.createElement('td');
        const timeCell = document.createElement('td');
        const bar = document.createElement('div');
    
        const link = document.createElement('a');
        link.href = `#cmd${index + 1}`;
        link.textContent = command.name;
        link.style.display = 'block';
        link.style.textAlign = 'center';    
        commandCell.appendChild(link);

        bar.className = 'timer-bar';
        bar.style.width = `${(command.time / sum) * 100}%`;
        bar.style.marginLeft = `${accumulatedTime / sum * 100}%`;
        bar.style.backgroundColor = command.color;
        bar.setAttribute('data-time', command.time);
    
        timeCell.appendChild(bar);
        row.appendChild(commandCell);
        row.appendChild(timeCell);
        tableBody.appendChild(row);
    
        accumulatedTime += command.time;
    }, 0);
    </script>
</body>
</html>)";

    return html;
}

std::string run_command(const std::vector<std::string>& commands) {

    timer::time_point<std::chrono::system_clock> start, end;

    std::string time;
    std::array<char, 128> buffer;
    std::string final_result;
    int index = 0;
    for (const auto& command : commands) {
        std::cout << command << "\n";
        std::string result;
        std::string full_command = "script -q -c \"" + command + "\" 2>&1";

        start = timer::system_clock::now();
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(full_command.c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }

        end = timer::system_clock::now();
        auto elapsed_seconds = end - start;

        double ms = timer::duration<double, std::milli>(elapsed_seconds).count();

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        std::cout << result << "\n";

        std::string html_output = ansi_to_html(result);
        std::string pre_content = extract_pre_content(html_output);



        std::string start_tags = std::format(R"(<div class="command-box">
            <span class="ansi2" id = "cmd{}"> {} </span>
            </div>
            <div class="error-box">)", ++index, command);
        
        if (pre_content.length() > 0)
            pre_content = start_tags + pre_content + "</div><br><br>";

        final_result += pre_content;

        bool is_err = false, is_warn = false;

        is_err = get_exit_code("typescript") != 0;

        if (result.find("warning") != std::string::npos || result.find("Warning") != std::string::npos || result.find("WARNING") != std::string::npos) {
            is_warn = true;
        }

        std::string color = is_err ? "#FF5733" : (is_warn ? "#FFC300" : "#4CAF50");

        std::ostringstream ostr;

        ostr << "{ name: \'" << command  << "\', time: " << ms << ", color: \'" << color << "\' },";

        time += ostr.str();

        if(is_err) break;
    }

    std::remove("typescript");

    return create_html(final_result, time);
}