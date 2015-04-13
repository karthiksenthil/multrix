files =  Dir.glob("test*\.c")
files.each do |f|
	out = f.split('.')[0]
	`gcc -o ./test_binaries/#{out} #{f}`
end
puts files
