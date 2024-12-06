CONTRIBUTIONS
Team Members: Katherine Guerrerio (kguerre6) and Angela Guo (aguo14)
We used VSCode Live Share to work on almost all functions together.

Milestone 1 --------------------------------------------------------

Katherine:
- Table
- Message & MessageSerialization
- Client program functionality (get_value, set_value, and incr_value)

Angela:
- ValueStack
- Message & MessageSerialization
- Client program functionality (get_value, set_value, and incr_value)

Milestone 2 --------------------------------------------------------

Katherine:
- auto-commit mode, error handling
- explicit transactions
- Server, ClientConnection
- Report on synchronization

Angela:
- auto-commit mode, error handling
- Server, ClientConnection
- Report on synchronization

EXPLANATION
The server has a tables map that holds all the tables created. Since multiple threads can create/access tables simultaneously,
this map needs to be synchronized to prevent data corruption. We hence used a std::mutex (tables_mutex) to synchronize access.
Each method that accesses/modifies tables (create_table, find_table, print_tables) uses a std::lock_guard to lock the mutex,
which ensures the mutex is locked at the start of the function and automatically released when the function scope ends.
std::lock_guard ensures locks are always acquired and released properly, there are no code paths where the shared data structures
are accessed without a lock, and std::lock_guard eliminates the possibility of forgetting to release a lock, hence why we are
confident the server is free of race conditions. (ADD DEADLOCK REASONING)