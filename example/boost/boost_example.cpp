#include <boost/algorithm/string.hpp>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
  std::string text = "  Hello, Boost Algorithm Library!  ";

  // Trim whitespace from the string
  boost::algorithm::trim(text);
  std::cout << "Trimmed text: '" << text << "'" << std::endl;

  // Convert to uppercase
  std::string upper_text = boost::algorithm::to_upper_copy(text);
  std::cout << "Uppercase: " << upper_text << std::endl;

  // Convert to lowercase
  std::string lower_text = boost::algorithm::to_lower_copy(text);
  std::cout << "Lowercase: " << lower_text << std::endl;

  // Split the string into words
  std::vector<std::string> words;
  boost::algorithm::split(words, text, boost::is_any_of(" ,!"));
  std::cout << "Split words:\n";
  for (const auto &word : words) {
    if (!word.empty()) {
      std::cout << "- " << word << std::endl;
    }
  }

  // Join words with a hyphen
  std::string joined_text = boost::algorithm::join(words, "-");
  std::cout << "Joined with hyphen: " << joined_text << std::endl;

  // Check if string starts or ends with a certain substring
  if (boost::algorithm::starts_with(text, "Hello")) {
    std::cout << "The text starts with 'Hello'." << std::endl;
  }

  if (boost::algorithm::ends_with(text, "Library!")) {
    std::cout << "The text ends with 'Library!'." << std::endl;
  }

  return 0;
}