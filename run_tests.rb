tests = Dir["/home/karthik/Documents/HPC/New_Approach/Test_codes/test_binaries/*"]
tests.each do |t|
	temp = t.split('/')
	t = temp[temp.length-1]	

	`../../../pin -t obj-intel64/dependency_analysis.so -- /home/karthik/Documents/HPC/New_Approach/Test_codes/test_binaries/#{t} > ./test_analysis/out_#{t}`
	`gnuplot plotting_data1.gnu`
	`mv mem_access.png #{t}.png`
	`mv #{t}.png ./memory_access_results/`
end