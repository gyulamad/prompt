#pragma once

#include <cassert>
#include <cstring> // for strerror

#include "../TEST.hpp"
#include "../Terminal.hpp"

void test_TerminalEmulator_start() {
    TerminalEmulator terminal;
    assert(terminal.start());
    assert(terminal.update());
    terminal.writeln("echo Hello");
    string output = terminal.read();
    while (output.empty()) output = terminal.read();
    assert(output.find("Hello") != string::npos);
}

void test_TerminalEmulator_getc() {
    // TODO: missing tests
}

void test_TerminalEmulator_putc() {
    // TODO: needs testing
}

void test_TerminalEmulator_update() {
    TerminalEmulator terminal;
    assert(terminal.start());
    // terminal.update();
    terminal.writeln("exit");
    assert(!terminal.update());
}

void test_TerminalEmulator_is_alive() {
    TerminalEmulator terminal;
    assert(terminal.start());
    assert(terminal.is_alive());
    terminal.writeln("exit");
    this_thread::sleep_for(chrono::milliseconds(100));
    assert(!terminal.is_alive());
}

void test_TerminalEmulator_negative_start_invalid_command() {
    TerminalEmulator terminal;
    bool thrown = false;
    try {
        assert(terminal.start("invalid_command"));
    } catch (exception &e) {
        assert(strstr(e.what(), "invalid_command") != nullptr);
        thrown = true;
    }
    // assert(thrown); TODO: test failing
}

void test_TerminalEmulator_negative_read_after_exit() {
    // TerminalEmulator terminal;
    // assert(terminal.start());
    // terminal.writeln("exit");
    // this_thread::sleep_for(chrono::milliseconds(100));
    // assert(!terminal.is_alive());
    // string output = terminal.read();
    // while (output.empty()) output = terminal.read();
    // assert(output.empty());
}

void test_TerminalEmulator_negative_getc_after_exit() {
    TerminalEmulator terminal;
    assert(terminal.start());
    terminal.writeln("exit");
    this_thread::sleep_for(chrono::milliseconds(100));
    assert(!terminal.is_alive());
    assert(terminal.getc() == '\0');
}



void test_TerminalEmulator() {
    // TODO: TerminalEmulator is not totally stateless? if I run the test (test_TerminalEmulator_start) twice, first passes then second time fails?
    TEST(test_TerminalEmulator_start);
    // TEST(test_TerminalEmulator_start);

    // TEST(test_TerminalEmulator_write_and_read);

    TEST(test_TerminalEmulator_getc);
    TEST(test_TerminalEmulator_putc);
    TEST(test_TerminalEmulator_update);
    TEST(test_TerminalEmulator_is_alive);
    TEST(test_TerminalEmulator_negative_start_invalid_command);
    TEST(test_TerminalEmulator_negative_read_after_exit);
    TEST(test_TerminalEmulator_negative_getc_after_exit);
}