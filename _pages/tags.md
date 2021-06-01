---
layout: single
#classes: wide
toc: true
title:  "DIY Word Clock"
date:   2020-01-11 20:00:00 +0200
categories: blog
tags: 
- IoT
- DIY
---

These are all tags:

{% for tag in site.tags %}
  <h3>{{ tag[0] }}</h3>
  <ul>
    {% for post in tag[1] %}
      <li><a href="{{ post.url }}">{{ post.title }}</a></li>
    {% endfor %}
  </ul>
{% endfor %}