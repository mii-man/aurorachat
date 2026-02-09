<h1 align="center">aurorachat</h1>
<p align="center">A chatting application for the Nintendo 3DS and 2DS line of systems</p>
<p align="center">aurorachat is currently in a drout in terms of activity and development. We are working on major changes but please do be aware that many things will happen soon, but not yet!</p>


<div align="center">
  <a href="https://tinyurl.com/auroracord">Join our Discord!</a> &nbsp;&nbsp;&nbsp;
  <a href="https://github.com/ItsFuntum/aurorachat-wiiu">Wii U client</a>
</div>

‎
<div align="center">
  <h2>Where can I download stable builds?</h2>
  <p>Go to our <a href="https://github.com/mii-man/aurorachat/releases">GitHub Releases!</a></p>
</div>
<div align="center">‎ 
  ‎ <h2>Where can I get immediate, easy builds?</h2>
  ‎ <p>Github Actions automatically provides us with builds! <a href="https://github.com/mii-man/aurorachat/actions/workflows/build.yml">Choose the latest run and download the build in the Summary page!</a></p>
</div>

<h1 align="center">Welcome to the aurorachat repository!</h1>
This repository is <b>open</b> for contributions! If you'd like to, you may open a PR or an issue, contributing helps us as we develop aurorachat!
‎ 
<div align="center">
  <details>
  <summary><strong>Development Log</strong></summary>
  
  | Version | Status | Changelog |
  |--------|--------|--------|
  | v0.0.4.0 | In development | TLS verification, accounts, audio, GUI overhaul |
  | v0.0.3.9 | Maybe it was the friends we made along the way | Encryption, console-specific IDs |
  | v0.0.3.4 | Released | PC Client, security improvements as well as a stack increase |
  | v0.0.3.3 | Released | Fixed crash from duplicate theme text |
  | v0.0.3.2 | Released | Added 3 themes, improved theme manager |
  | v0.0.3.1 | Released | Added 2 themes, code fixes |
  | v0.0.3 | Released | Added theme support |
  | v0.0.2.5 | Released | Extended name length |
  | v0.0.2.4 | Released | Fixed metadata implementation |
  | v0.0.2.3 | Released | Initial metadata support |
  | v0.0.2.2 | Released | Fixed RAM issue (audio temporarily removed) |
  | v0.0.2.1 | Released | Fixed 3DS audio playback |
  | v0.0.2 | Released | Full code revamp 2 with secure hbchat base |
  | v0.0.1 | Unreleased | Full Code Revamp due to me realising that citro2d has better text than the console. Also added a messsage sound from LINE (the texting app) to make the message sound. (Edit as of 10/22/25 AEDT: I'm using self made sound effects for sending and receiving messages) |
  | v0.0.0 | Unreleased | I have no idea. |
  
  </details>
</div>
‎ 
<h1 align="center">How to build aurorachat</h1>

Install devkitpro with the 3DS development libraries and make, then execute the following commands based on your OS:

Windows:
```sh
pacman -S 3ds-opusfile
git clone https://github.com/mii-man/aurorachat
cd aurorachat
make
make cia
```

Arch Linux or other distros with pacman:
```sh
sudo pacman -S 3ds-opusfile
git clone https://github.com/mii-man/aurorachat
cd aurorachat
make
make cia
```

Other Linux distros without pacman:
```sh
sudo dkp-pacman -S 3ds-opusfile
git clone https://github.com/mii-man/aurorachat
cd aurorachat
make
make cia
```

(At least that's what I think you gotta do)

## checklist if it was evil
- [X] basic chatting 
- [X] awesome themes
- [ ] awesome encryption
- [X] account stuff
- [ ] other stuff
- [X] sound stuff
- [ ] advanced chating
- [ ] and all the stuff that we forgot
