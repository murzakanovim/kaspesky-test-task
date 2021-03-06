#include <filesystem>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <fstream>
#include <sstream>

namespace task
{
  namespace fs = std::filesystem;
  bool starts_with(const std::string& smaller_string, const std::string& bigger_string)
  {
    return (smaller_string == bigger_string.substr(0, smaller_string.length()));
  }

  std::string convert_one_line(const std::string& gmi_line)
  {
    static const std::map<std::string, std::string> tags_map // Map of regular expressions which helps identify type of line
    {
        {"^# (.*)", "h1"}, // #<whitespace><text>
        {"^## (.*)", "h2"}, // ##<whitespace><text>
        {"^### (.*)", "h3"}, // ###<whitespace><text>
        {"^\\* (.*)", "li"}, // *<whitespace><text>
        {"^> (.*)", "blockquote"}, // ><whitespace><text>
        {"^=>\\s*(\\S+)(\\s+.*)?", "a"} // =>[<whitespace>]<URL>[<whitespace><USER-FRIENDLY LINK NAME>]
    };

    std::string result;

    for (const auto& tags_pair : tags_map)
    {
      if (std::regex_match(gmi_line, std::regex(tags_pair.first)))
      {
        std::string tag = tags_pair.second;
        std::stringstream ss(gmi_line);
        if (tag == "a") // checking for a link tag
        {
          std::string href;
          std::string reference_name;

          char skippedChar;
          ss >> skippedChar >> skippedChar; // skip "=>"

          ss >> href;
          getline(ss >> std::ws, reference_name);

          result = "<" + tag + " href=\"" + href + "\">" + reference_name + "</" + tag + ">";
          return result;
        }
        std::string text;
        ss >> text; // skip special characters
        std::getline(ss >> std::ws, text);
        result = "<" + tag + ">" + text + "</" + tag + ">";
        return result;
      }
    }
    result = "<p>" + gmi_line + "</p>";
    return result;
  }

  void gmiToHtml(const fs::path& src, const fs::path& target) // src - path to gmi file, target - path to result html file
  {
    std::ifstream gmi_file(src);
    std::ofstream html_file(target);

    std::string gmi_line;
    std::string html_line;

    bool is_preformatted = false;
    bool is_list = false;

    while (std::getline(gmi_file, gmi_line))
    {
      if (starts_with("```", gmi_line)) // checking for on/off preformatted mode
      {
        is_preformatted = !is_preformatted;
        std::string pre_tag = is_preformatted ? "<pre>" : "</pre>";
        html_file << pre_tag << "\n";
      }
      else if (is_preformatted)
      {
        html_file << gmi_line << "\n";
      }
      else
      {
        html_line = convert_one_line(gmi_line);
        if (starts_with("<li>", html_line)) // checking for on/off list mode
        {
          if (!is_list)
          {
            is_list = true;
            html_file << "<ul>\n";
          }
        }
        else if (is_list)
        {
          is_list = false;
          html_file << "</ul>\n";
        }
        html_file << html_line << "\n";
      }
    }
    if (is_list)
    {
      html_file << "</ul>\n";
    }
    html_file.close();
  }

  void generateSite(const fs::path& input_directory, const fs::path& output_directory)
  {
    try
    {
      fs::create_directory(output_directory); // creating directory if it does not exist
      for (const auto& dir_entry : fs::recursive_directory_iterator(input_directory))
      {
        const fs::path current_path = dir_entry.path();
        const fs::path relative_path = fs::relative(current_path, input_directory); // used when creating a directory or when copying a non gmi file
        if (dir_entry.is_directory())
        {
          fs::create_directories(output_directory / relative_path);
        }
        else
        {
          if (current_path.extension() == ".gmi")
          {
            const fs::path relative_html_file_path = fs::relative(current_path, input_directory).parent_path();
            const std::string html_filename = current_path.stem().string() + ".html";
            const fs::path output_html_path = output_directory / relative_html_file_path / html_filename;
            gmiToHtml(current_path, output_html_path);
          }
          else
          {
            fs::copy(current_path, output_directory / relative_path, fs::copy_options::overwrite_existing);
          }
        }
      }
    }
    catch (const std::exception& exc)
    {
      std::cerr << exc.what();
    }
  }
}

int main()
{
  std::filesystem::path input_directory;
  std::filesystem::path output_directory;
  std::cin >> input_directory >> output_directory;
  task::generateSite(input_directory, output_directory);
}
