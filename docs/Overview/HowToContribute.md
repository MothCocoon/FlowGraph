---
title: How To Contribute
---

### Contribute to plugin's code
We are here very open to accept plugin improvements via pull requests. At the moment of writing this words, we got 60 contributors with accepted pull requests. Over 200 pull requests have been resolved.

* Don't be shy to ask on our Discord channel "hey, would this kind of code change would be accepted?". This might indeed save you some time, especially if Flow Graph is a new thing for you and your team.
* Feel free to open new pull request, even if you are quite unsure if given change would be accepted. It's often easier to discuss technicalities while proposed code change can be accessed by community.

[Pull requests](https://github.com/MothCocoon/FlowGraph/pulls)

### Contribute to documentation
The entire plugin documentation has been moved from the GitHub wiki to GitHub Pages. The most important advantage is that now anybody can contribute to improving documentation via pull requests.

Contributing to docs is quite easy. There are a few things to know.
* Every documentation page is a separate Markdown file: a text file with .md extension. Example: this page is written as HowToContribute.md.
    * Markdown is a common markup language used to write wikis and documentation. You will find plenty of guides, cheet sheets for Markdown on the web.
    * All pages live in `FlowGraph/docs` folder, organized in a few folders like `Overview`, `Features`, `Guides`. You might already notices that folder names match the structure of sidebar links.
* If you'd like add to a new page.
    * Please remember to add new pages in one of existing folders. And let's talk on Discord, if you think we need a new folder (a new page category).
    * A link to a new page must be added manually by editing `_data/navigation.yml`. It's one of files expected by Jekyll-based site (the thing powering GitHub Pages).
    * The only element required in a new page is to include "title" section at the top of Markdown file. Please check how existing files have it defined.
* If you'd like embed image or other file. Files can be uploaded to GitHub servers, but it is quite unintuitive.
    * Go to GitHub repository on GitHub.com. Go to Issues, Pull Requests or Wiki.
    * Create draft of a new issue, or start writing comment on existing issue, or comment on exisint pull request. Don't worry, you don't need to send it.
    * Just drag your image into text editing area. GitHub will automatically upload the image. Copy-paste the generated link to the Markdown file you're editing locally.
    * Discard draft of whatever text you were editing on GitHub.com.
    * It's cumbersome way, but this way we avoid bloating the disk size of the Flow Graph repository. Plugin itself consumes 2MB of disk size. Adding images to the repository would quickly consume hundreds of MBs.
