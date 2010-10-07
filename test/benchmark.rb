require 'socket'

def single_test(test)
	time_start = Time.now.to_f

	File.open(test, 'r') { |file|
		@sock = TCPSocket.new('www.michal', 6974)
		# Send file
		@sock.puts 'searcher_userid 3233577'
		@sock.puts 'searcher_school 0'
		@sock.puts 'searcher_location 0'
		while (line = file.gets)
			@sock.puts line
		end
		@sock.puts 'END'
		# Read result
		count = 0
		while (line = @sock.gets)
			# puts line
			count += 1
		end
		if (count == 0)
			puts "Warning, did not read any results"
		end
		@sock.close
	}
	
	time_taken = Time.now.to_f - time_start
#	puts '%.5f' % time_taken
	return time_taken
end

puts $0
if ARGV.length < 2
	puts "Need command-line arguments:"
	puts "	<iters> <test1> <test2> ..."
	exit!
end

ITERS = ARGV[0].to_i || 1000
tests = ARGV[1..-1]

total_time = 0.0
for i in 0...ITERS
	tests.each { |test|
		total_time += single_test(test)
	}
end
puts "Total time: #{'%.5f' % total_time}, avg time: #{'%.5f' % (total_time / ITERS)}"
