# ats_syncer
A tool to sync sector files and share announcements across mappers in an ATS/(ETS2?) project using one central controlling server.
### Server Installation
1. `git clone` the repository to a folder on your server, and `rm -r` the clients/ directory.
2. Edit `auth.php` and add your username/password combinations. By default, each of these fields has to be <= 16 characters. The username field can only contain letters and the password field must be alphanumeric. Add any administrator-level users to the array.
### Client Installation
1. `git clone` the repository. Open it in Visual Studio 2026.
2. Edit `INTERNAL_DEF_HOST_BASE` in `store_announce.h` to be the URL of your server. This must include the `http://` prefix.
3. Build the release version and send to other mappers.
You/other mappers can place the client executable anywhere you want, since  the AppData directory is used to store persistent settings.
