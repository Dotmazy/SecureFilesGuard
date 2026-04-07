$cert = Get-ChildItem Cert:\LocalMachine\My | Where-Object {$_.Subject -like "*FolderGuard*"}
Set-AuthenticodeSignature -FilePath "FolderGuardDriver.sys" -Certificate $cert