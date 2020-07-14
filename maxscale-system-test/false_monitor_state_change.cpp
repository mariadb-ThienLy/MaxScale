/**
 * Test false server state changes when manually clearing master bit
 */

#include <maxtest/testconnections.hh>

int main(int argc, char* argv[])
{
    TestConnections test(argc, argv);

    test.tprintf("Block master");
    test.repl->block_node(0);

    test.tprintf("Wait for monitor to see it");
    test.maxscales->wait_for_monitor();

    test.tprintf("Clear master status");
    test.maxctrl("clear server server1 master");
    test.maxscales->wait_for_monitor();

    test.repl->unblock_node(0);
    test.maxscales->wait_for_monitor();

    return test.global_result;
}
