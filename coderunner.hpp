#ifndef CODERUNNER_HPP
#define CODERUNNER_HPP

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
#include <iomanip>
#include <cctype>
#include <algorithm>
#include <future>
#include <thread>
#include <mutex>

namespace timer = std::chrono;

namespace py = pybind11;

std::mutex py_mutex, stdout_mutex;

/**
 * @brief Extracts the exit code from the last line of a file.
 * 
 * @param file_path The path to the file containing the exit code.
 * @return The extracted exit code as an integer.
 * @throws std::runtime_error If the file cannot be opened or the exit code is not found.
 */
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
        std::cout << file_path << std::endl;
        throw std::runtime_error("Exit code not found in the last line");
    }
}

/**
 * @brief Converts ANSI text to HTML using the Python `ansi2html` library.
 * 
 * @param ansi_text The ANSI-formatted text to be converted.
 * @return The converted HTML string.
 */
std::string ansi_to_html(const std::string& ansi_text) {
    std::lock_guard<std::mutex> lock(py_mutex);
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

/**
 * @brief Extracts the content inside `<pre>` tags from an HTML string.
 * 
 * @param html The HTML string to process.
 * @return The extracted content inside `<pre>` tags, or an empty string if no content is found.
 */
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

/**
 * @brief Creates an HTML document containing build results and a Gantt chart.
 * 
 * @param output The HTML content for the build results.
 * @param commands The commands and their execution details for the Gantt chart.
 * @return The complete HTML document as a string.
 */
std::string create_html(const std::string& output, std::string& commands) {
    std::string html = R"(<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
  <title>Forge Build Timeline</title>

  <script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>

  <style type="text/css">
    .ansi2html-content { display: inline; white-space: pre-wrap; word-wrap: break-word; }
    .body_foreground { color: #f5f3f3; }
    .body_background { background-color: #ffffff; }
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
      margin-bottom: 0;
    }

    .forge-box {
      border: 2px solid #0e0e0e;
      padding: 5px;
      color: black;
      background-color:  #f6f6f7;
      margin-bottom: 0;
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

    .gantt-header {
      display: flex;
      font-weight: bold;
      background-color: #600780;
      color: white;
      padding: 8px 12px;
    }

    .gantt-header .command-col {
      width: 25%;
      min-width: 120px;
    }

    .gantt-header .time-col {
      flex: 1;
    }

    #chart_div {
      width: 100%;
    }

    html, body {
      margin: 0;
      padding: 1rem;
    }
  </style>
</head>

<body class="body_foreground body_background">

  <h1 class="forge-box">
    <a href="https://github.com/Kronos-192081/forge">forge</a> build results
  </h1>

  <h2 style="color: black;">Build Summary:</h2>
  <div class="gantt-header">
    <div class="command-col">Command</div>
    <div class="time-col">Timeline</div>
  </div>
  <div id="chart_div"></div>

  <br>

  <h2 style="color: black;">Compilation Details:</h2>)";

html += output;
 
html += R"(<script>
    google.charts.load('current', { packages: ['gantt'] });
    google.charts.setOnLoadCallback(drawChart);

    function drawChart() {
      const container = document.getElementById('chart_div');
      const width = container.getBoundingClientRect().width;
      const rowHeight = 30;

      const data = new google.visualization.DataTable();
      data.addColumn('string', 'Task ID');
      data.addColumn('string', 'Command');
      data.addColumn('string', 'Resource');
      data.addColumn('date', 'Start');
      data.addColumn('date', 'End');
      data.addColumn('number', 'Duration');
      data.addColumn('number', 'Percent Complete');
      data.addColumn('string', 'Dependencies');

      const rows = [)";

html += commands;

html += R"(];

      data.addRows(rows);

      const colorMap = {
        'green': '#4CAF50',
        'red': '#F44336',
        'yellow': '#FFC107'
      };

    const palt = [];
    const colorSet = new Set();
    for (const row of rows) {
      const color = colorMap[row[2]];
      if (!colorSet.has(color)) {
        obj = {
        color: color,
        dark: color,
        light: color
        };
        palt.push(obj);
        colorSet.add(color);
      }
    }

      console.log(palt);

      const options = {
        height: rows.length * rowHeight + 50,
        width: width,
        gantt: {
          trackHeight: rowHeight,
          palette: palt,
        }
      };

      const chart = new google.visualization.Gantt(container);
      chart.draw(data, options);

      // Clickable bars
      const commandLinks = {};
        rows.forEach((row, index) => {
        const command = row[1];
        commandLinks[command] = `#cmd${index + 1}`;
     });

      google.visualization.events.addListener(chart, 'select', function () {
        const selection = chart.getSelection();
        if (selection.length > 0) {
          const row = selection[0].row;
          const command = data.getValue(row, 1);
          const link = commandLinks[command];
          if (link) {
            location.href = link;
          }
        }
      });
    }

    window.addEventListener('resize', drawChart);
  </script>
</body>
</html>)";

    return html;
}


auto base = std::chrono::system_clock::from_time_t(0);

/**
 * @brief Formats a `std::chrono::system_clock::time_point` into a JavaScript `Date` object string.
 * 
 * @param tp The time point to format.
 * @return A string representing the JavaScript `Date` object.
 */
std::string formatDateTime(const std::chrono::system_clock::time_point& tp) {
    using namespace std::chrono;

    auto duration = tp - base;
    auto millis = duration_cast<milliseconds>(duration).count();

    auto hours = millis / (1000 * 60 * 60);
    millis %= (1000 * 60 * 60);
    auto minutes = millis / (1000 * 60);
    millis %= (1000 * 60);
    auto seconds = millis / 1000;
    auto milliseconds_part = millis % 1000;

    std::ostringstream oss;
    oss << "new Date(1970, 0, 1, "
        << hours << ", "
        << minutes << ", "
        << seconds << ", "
        << milliseconds_part << ")";

    return oss.str();
}


int counter = 0;

/**
 * @brief Executes a list of commands sequentially and captures their output.
 * 
 * @param commands A vector of commands to execute.
 * @return A tuple containing:
 * - The HTML-formatted output of the commands.
 * - The timeline data for the Gantt chart.
 * - A boolean indicating if any command failed.
 */
std::tuple<std::string, std::string, bool> run_command(const std::vector<std::string>& commands) {

    timer::time_point<std::chrono::system_clock> start, end;

    std::string time;
    std::array<char, 128> buffer;
    std::string final_result;
    bool err = false;
    int index = 0;
    for (const auto& command : commands) {
        counter++;
        std::string result;
        std::string full_command = "script -q -c \"" + command + "\"" + " 2>&1";

        std::cout << command << std::endl; 

        start = timer::system_clock::now();
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(full_command.c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        
        std::cout << result << std::endl;

        std::string html_output = ansi_to_html(result);
        std::string pre_content = extract_pre_content(html_output);

        std::string start_tags = std::format(R"(<div class="command-box">
            <span class="ansi2" id = "cmd{}"> {} </span>
            </div>
            <div class="error-box">)", counter, command);
        
        if (pre_content.length() > 0)
            pre_content = start_tags + pre_content + "</div><br><br>";

        final_result += pre_content;
        bool is_err = false, is_warn = false;
        is_err = get_exit_code("typescript") != 0;
        if (is_err) err = true;

        if (result.find("warning") != std::string::npos || result.find("Warning") != std::string::npos || result.find("WARNING") != std::string::npos) {
            is_warn = true;
        }

        end = timer::system_clock::now();

        std::string color = is_err ? "red" : (is_warn ? "yellow" : "green");
        std::ostringstream ostr;
        ostr << "[" << "\'" << counter << "\' , " <<  "\'" << command <<  "\' , " << "\'" << color << "\' , " << formatDateTime(start) << " , " << formatDateTime(end) << ", null , " << (is_err ? 0 : 100) << " , null ],";
        time += ostr.str();

        if(is_err) break;

        std::remove("typescript");
    }

    std::remove("typescript");

    return {final_result, time, err};
}

/**
 * @brief Executes a list of commands sequentially and generates an HTML report.
 * 
 * @param commands A vector of commands to execute.
 * @return The HTML report as a string.
 */
std::string run_commands(const std::vector<std::string>& commands){
    std::string time;
    std::string final_result;
    bool is_err = false;

    using timer = std::chrono::system_clock;
    timer::time_point start, end;

    start = timer::now();
    for (const auto& command : commands) {
        auto [result, command_time, err] = run_command({command});
        final_result += result;
        time += command_time;
        if (err) { is_err = true; break; }
    }
    end = timer::now();
    auto elapsed_seconds = std::chrono::duration<double>(end - start).count();

    if (!is_err){
        std::cout << "\033[34m" << "Compilation process completed in: " << elapsed_seconds << " seconds" << "\033[0m" << std::endl;
    } else {
        std::cout << "\033[31m" << "Compilation process completed in: " << elapsed_seconds << " seconds" << "\033[0m" << std::endl;
    }

    return create_html(final_result, time);
}

/**
 * @brief Executes a list of commands sequentially in parallel and captures their output.
 * 
 * @param commands A vector of commands to execute.
 * @return A tuple containing:
 * - The HTML-formatted output of the commands.
 * - The timeline data for the Gantt chart.
 * - A boolean indicating if any command failed.
 */
std::tuple<std::string, std::string, bool> run_command_par(const std::vector<std::string>& commands) {

    timer::time_point<std::chrono::system_clock> start, end;

    std::string time;
    std::array<char, 128> buffer;
    std::string final_result;
    bool err = false;
    int index = 0;
    for (const auto& command : commands) {
        std::string result;
        std::string full_command = command +" 2>&1";

        start = timer::system_clock::now();
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(full_command.c_str(), "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }

        {
            std::lock_guard<std::mutex> lock(stdout_mutex);
            std::cout << command << std::endl;
            std::cout << result << std::endl;
        }

        std::string html_output = ansi_to_html(result);
        std::string pre_content = extract_pre_content(html_output);
        bool is_err = false, is_warn = false;

        auto pip = pipe.release();
        is_err = pclose(pip) != 0;

        if (is_err) err = true;

        if (result.find("warning") != std::string::npos || result.find("Warning") != std::string::npos || result.find("WARNING") != std::string::npos) {
            is_warn = true;
        }

        end = timer::system_clock::now();

        std::string color = is_err ? "red" : (is_warn ? "yellow" : "green");
        std::ostringstream ostr;
        {
            std::lock_guard<std::mutex> lock(stdout_mutex);
            counter++;
            ostr << "[" << "\'" << counter << "\' , " <<  "\'" << command <<  "\' , " << "\'" << color << "\' , " << formatDateTime(start) << " , " << formatDateTime(end) << ", null , " << (is_err ? 0 : 100) << " , null ],";
            time += ostr.str();
            std::string start_tags = std::format(R"(<div class="command-box">
            <span class="ansi2" id = "cmd{}"> {} </span>
            </div>
            <div class="error-box">)", counter, command);
        
            if (pre_content.length() > 0)
                pre_content = start_tags + pre_content + "</div><br><br>";

            final_result += pre_content;
        }

        if(is_err) break;
    }

    return {final_result, time, err};
}

/**
 * @brief Executes batches of commands in parallel using multiple threads and generates an HTML report.
 * 
 * @param command_batches A vector of command batches, where each batch is a vector of command groups.
 * @param num_threads The maximum number of threads to use for parallel execution.
 * @return The HTML report as a string.
 */
std::string run_commands_parallel(const std::vector<std::vector<std::vector<std::string>>>& command_batches, size_t num_threads) {
    std::string final_result, total_time;
    std::mutex result_mutex;
    std::vector<std::thread> threads;
    std::atomic<bool> is_err(false);

    using clock = std::chrono::system_clock;
    auto start = clock::now();

    for (const auto& batch : command_batches) {
        std::vector<std::future<void>> futures;
        is_err.store(false);

        for (const auto& command : batch) {
            futures.emplace_back(std::async(std::launch::async, [&command, &final_result, &total_time, &result_mutex, &is_err]() {
                if (is_err.load()) {
                    return;
                }

                auto [result, command_time, err] = run_command_par(command);

                {
                    std::lock_guard<std::mutex> lock(result_mutex);
                    final_result += result;
                    total_time += command_time;
                }

                if (err) {
                    is_err.store(true);
                    return;
                }
            }));

            if (futures.size() >= num_threads) {
                for (auto& future : futures) {
                    future.get();
                }
                futures.clear();
            }
        }

        for (auto& future : futures) {
            future.get();
        }

        if (is_err.load()) {
            break;
        }
    }

    auto end = clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << (is_err.load() ? "\033[31m" : "\033[34m")
            << "Compilation process completed in: " << elapsed << " seconds"
            << "\033[0m" << std::endl;

    return create_html(final_result, total_time);
}

#endif // CODERUNNER_HPP
