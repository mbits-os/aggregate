Aggregate
=========

## Developement Environment ##

<dl>
<dt>apache/apache.conf</dt>
<dd>Change <code>F:/code/</code> in both <code>F:/code/aggregate/apache/httpd</code> to where you've cloned Aggregate.</dd>
<dd>Change <code>ServerName</code> to your developement server.</dd>

<dt>platforms/win32/copy_index.cmd</dt>
<dd>Change the name of Apache service to proper version. You can check the name of your instlled service in <em>Management Console</em>, under <em>Services and Application</em>, and then <em>Services</em>. The name will be in the column, well, <em>Name</em>.</dd>

<dt>apache/conn.ini</dt>
<dd>Tough luck. No can do. <em>But</em>, if can lay your hands on a MySQL database, you need to have those entries in the <code>conn.ini</code> file:
<pre><code>driver=mysql
user=<em>username</em>
password=<em>password</em>
server=<em>server|server:port</em>
database=<em>db at server</em></code></pre></dd>

<dt>Command Prompt</dt>
<dd><pre><code>platforms\win32&gt; projects.cmd
platforms\win32&gt; start Aggregate.sln</code></pre></dd>
</dl>
