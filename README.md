# ats_syncer
A tool to sync sector files and share announcements across mappers in an ATS/(ETS2?) project using one central controlling server.
### Server Installation
1. `git clone` the repository to a folder on your server. You may optionally remove the `clients/` directory.
2. Edit `auth.php` and add your username/password combinations. By default, each of these fields has to be <= 16 characters. The username key can only contain letters and each password value must be alphanumeric. Add any administrator-level users to the array.
### Client Installation
1. `git clone` the repository to a local folder.
2. You may optionally remove the `server/` directory
3. Open `clients/ats_syncer_client.slnx` in Visual Studio 2026.
4. Edit `INTERNAL_DEF_HOST_BASE` in `store_announce.h` to be the public-facing URL of the folder you created on your server. This must include the `http://` prefix. Do not include a / at the end of the URL, or requests will fail.
5. Build the release version and distribute to other mappers.
You/other mappers can place the client executable anywhere you want, since the AppData directory is used to store persistent settings.
Do not connect to untrusted hosts: The client is written in unsafe C and by no means is this code 100% secure.
