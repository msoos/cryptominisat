Add-Type -AssemblyName System.IO.Compression.FileSystem

function Unzip
{
    param([string]$zipfile, [string]$outpath)

    [System.IO.Compression.ZipFile]::ExtractToDirectory($zipfile, $outpath)
}

$wc = New-Object System.Net.WebClient
$wc.DownloadFile("https://boostorg.jfrog.io/artifactory/main/release/1.80.0/source/boost_1_80_0.zip", "C:\projects\cryptominisat\boost_1_80_0.zip")

Unzip "C:\projects\cryptominisat\boost_1_59_0.zip" "C:\projects\cryptominisat"
