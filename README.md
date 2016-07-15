Wait, what is this?
===================

Hey there. This is some code for an assignment I had to do when I was studying abroad at the Australian National University. We were required to create a simple proxy to allow IPv4-only clients to connect to an IPv6-only PostgreSQL database. There are 2 distinct versions of this assignment in this repo. The first, *pg_proxy* is the simplest and acts as a really simple proxy. The other version, *pg_proxy2*, is a little more complex in which it actually intercepts messages to read usernames and looks up a config file (`config.txt`) to see if the username is allowed at the client IP.

Full documentation for this assignment is available at:
https://gitlab.cecs.anu.edu.au/comp3310/comp3310-2016-pg_proxy/wikis/home