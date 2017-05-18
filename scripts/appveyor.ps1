Add-Type -assembly "system.io.compression.filesystem"
$wc = New-Object System.Net.WebClient
$wc.DownloadFile("http://bit.ly/1JPHkL3", "C:\projects\cryptominisat\boost_1_59_0.zip")
[io.compression.zipfile]::ExtractToDirectory("C:\projects\cryptominisat\boost_1_59_0.zip", "C:\projects\cryptominisat\")
