#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <set>
#include <string_view>
#include <sstream>
#include <utility>
#include <ciso646>
#include <unistd.h>

#include "simdjson.h"
#include "cast_tester.h"
#include "test_macros.h"

const size_t AMAZON_CELLPHONES_NDJSON_DOC_COUNT = 793;
#define SIMDJSON_SHOW_DEFINE(x) printf("%s=%s\n", #x, STRINGIFY(x))

namespace stream_tests {
  using namespace simdjson;
  // static bool twitter_count() {
  //   std::cout << "Running " << __func__ << std::endl;
  //   // Prints the number of results in twitter.json
  //   dom::parser parser;
  //   auto [result_count, error] = parser.load_stream(TWITTER_JSON)["search_metadata"]["count"].get_unsigned();
  //   if (error) { cerr << "Error: " << error << endl; return false; }
  //   if (result_count != 100) { cerr << "Expected twitter.json[metadata_count][count] = 100, got " << result_count << endl; return false; }
  //   return true;
  // }

  static bool cars_count() {
    std::cout << "Running " << __func__ << std::endl;
    auto cars_json = R"( [
      { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
      { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
      { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
    ] )"_padded;
    dom::parser parser;

    // Parse and iterate through each car
    int count = 0;
    for (UNUSED stream::element &car : parser.stream(cars_json)) {
      count++;
    }
    if (count != 3) { std::cerr << "Expected count of cars to be 3, got " << count << std::endl; return false; }
    return true;
  }

  static bool average_tire_pressure_int() {
    std::cout << "Running " << __func__ << std::endl;
    auto cars_json = R"( [
      { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40, 39, 37, 40 ] },
      { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30, 31, 28, 28 ] },
      { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29, 30, 30, 30 ] }
    ] )"_padded;
    dom::parser parser;

    // Parse and iterate through each car
    for (stream::object car : parser.stream(cars_json)) {
      // Iterating through an array of floats
      uint64_t total_tire_pressure = 0;
      for (uint64_t tire_pressure : car["tire_pressure"]) {
        total_tire_pressure += tire_pressure;
      }
      std::cout << "- Average tire pressure: " << (total_tire_pressure / 4) << std::endl;
    }

    return true;
  }

  static bool average_tire_pressure() {
    std::cout << "Running " << __func__ << std::endl;
    auto cars_json = R"( [
      { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
      { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
      { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
    ] )"_padded;
    dom::parser parser;

    // Parse and iterate through each car
    for (stream::object car : parser.stream(cars_json)) {
      // Iterating through an array of floats
      double total_tire_pressure = 0;
      for (double tire_pressure : car["tire_pressure"]) {
        total_tire_pressure += tire_pressure;
      }
      std::cout << "- Average tire pressure: " << (total_tire_pressure / 4) << std::endl;
    }

    return true;
  }

  static bool newest_model() {
    std::cout << "Running " << __func__ << std::endl;
    auto cars_json = R"( [
      { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ] },
      { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ] },
      { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ] }
    ] )"_padded;
    dom::parser parser;

    // Parse and iterate through each car
    uint64_t newest_year = 0;
    std::string_view newest_make;
    std::string_view newest_model;
    for (stream::object car : parser.stream(cars_json)) {
      // NOTE this parses the string, but we have to do it before we read the year because we have
      // a single (hidden) iterator. Let's see if we can avoid the parse unless we choose the car;
      // a raw_json_string tied to the string writer will probably do fine.
      std::string_view make = car["make"];
      std::string_view model = car["model"];
      uint64_t year = car["year"];
      if (year > newest_year) {
        newest_make = make;
        newest_model = model;
        newest_year = year;
      }
    }
    std::cout << "A " << newest_make << " " << newest_model << " is the newest car, from " << newest_year << "." << std::endl;
    return true;
  }

  static bool busted_cars() {
    std::cout << "Running " << __func__ << std::endl;
    auto cars_json = R"( [
      { "make": "Kia",    "model": "Soul",   "year": 2012, "tire_pressure": [ 30.1, 31.0, 28.6, 28.7 ], "busted": false },
      { "make": "Toyota", "model": "Camry",  "year": 2018, "tire_pressure": [ 40.1, 39.9, 37.7, 40.4 ], "busted": true },
      { "make": "Toyota", "model": "Tercel", "year": 1999, "tire_pressure": [ 29.8, 30.0, 30.2, 30.5 ], "busted": false }
    ] )"_padded;
    dom::parser parser;

    // Parse and iterate through each car
    for (stream::object car : parser.stream(cars_json)) {
      // NOTE this parses the string, but we have to do it before we read the year because we have
      // a single (hidden) iterator. Let's see if we can avoid the parse unless we choose the car;
      // a raw_json_string tied to the string writer will probably do fine.
      std::string_view make = car["make"];
      std::string_view model = car["model"];
      if (car["busted"]) {
        std::cout << make << " " << model << " is busted!" << std::endl;
      }
    }
    return true;
  }

  static bool run() {
    return true
           && cars_count()
           && average_tire_pressure_int()
           && average_tire_pressure()
           && newest_model()
           && busted_cars()
    ;
  }
}

int main(int argc, char *argv[]) {
  std::cout << std::unitbuf;
  int c;
  while ((c = getopt(argc, argv, "a:")) != -1) {
    switch (c) {
    case 'a': {
      const simdjson::implementation *impl = simdjson::available_implementations[optarg];
      if (!impl) {
        fprintf(stderr, "Unsupported architecture value -a %s\n", optarg);
        return EXIT_FAILURE;
      }
      simdjson::active_implementation = impl;
      break;
    }
    default:
      fprintf(stderr, "Unexpected argument %c\n", c);
      return EXIT_FAILURE;
    }
  }

  // this is put here deliberately to check that the documentation is correct (README),
  // should this fail to compile, you should update the documentation:
  if (simdjson::active_implementation->name() == "unsupported") {
    printf("unsupported CPU\n");
  }
  // We want to know what we are testing.
  std::cout << "Running tests against this implementation: " << simdjson::active_implementation->name();
  std::cout << "(" << simdjson::active_implementation->description() << ")" << std::endl;
  std::cout << "------------------------------------------------------------" << std::endl;

  std::cout << "Running basic tests." << std::endl;
  if (true
      && stream_tests::run()
  ) {
    std::cout << "Basic tests are ok." << std::endl;
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}
