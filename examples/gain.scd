// Simplest possible script using all variables, demonstrating buffering
// by Brian Heim

~i = 0;
a = 0;
// ~process = {|x| a = max(a, bench { 12.do { 256.do { |i| sin(i)} }; post(x); }); post(a) }
~vcv_process = {post("test")}
