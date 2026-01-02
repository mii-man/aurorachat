<h1 align="center">aurorachat</h1>
<p align="center">A chatting application for the Nintendo 3DS and 2DS line of systems</p>


<div align="center">
  <a href="https://discord.gg/dCSgz7KERv">Join our Discord!</a> &nbsp;&nbsp;&nbsp;
  <a href="https://github.com/ItsFuntum/aurorachat-wiiu">Wii U client</a>
</div>

‎ 
<div align="center">‎ 
  ‎ <h2>Where can I get immediate, easy builds?</h2>
  ‎ <p>Github Actions automatically provides us with builds! <a href="https://github.com/mii-man/aurorachat/actions/workflows/build.yml">Choose the latest run and download the build in the Summary page!</a></p>
</div>
‎ 
‎ ‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
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
  | v0.0.2 | Released | Full code revamp with secure hbchat base |
  | v0.0.1 | Unreleased | Code revamp using citro2d (im not rewriting the yappery 😔) |
  | v0.0.0 | Unreleased | I have no idea. |
  
  </details>
</div>

‎ 
‎ 
‎ 
‎ 
‎ 
‎ ‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ ‎ 
‎ 
‎ 
‎ 
‎ 
‎ ‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 

<h1 align="center">Welcome to the aurorachat repository!</h1>
This repository is <b>open</b> for contributions! If you'd like to, you may open a PR or an issue, contributing helps us as we develop aurorachat!

‎ 
‎ 
‎ 
‎ 
‎ 
‎ ‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ ‎ 
‎ 
‎ 
‎ 
‎ 
‎ ‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
‎ 
<h1 align="center">How to build aurorachat</h1>

Install devkitpro with the 3DS dev tools, then execute the following commands (you'll have to adjust them if you're using mac or linux)

```sh
pacman -S 3ds-libopusfile
git clone https://github.com/mii-man/aurorachat
cd aurorachat
make
```

(At least that's what I think you gotta do)


## checklist if it was evil
- [X] basic chatting 
- [X] awesome themes
- [ ] awesome encryption
- [ ] account stuff
- [ ] other stuff
- [ ] sound stuff
- [ ] advanced chating
- [ ] and all the stuff that we forgot 
