<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:ia="http://www.iarchives.com/schema/2002/export"
  xmlns="http://www.w3.org/1999/xhtml"
  exclude-result-prefixes="ia">

  <xsl:output method='xml' version='1.0' encoding='utf-8' indent='yes'/>

  <!-- A page is a page... -->
  <xsl:template match="/ia:page">
    <html>
      <head>
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>
        <meta name="ocr-system" content="iarchive, transformed to be hOCR-like"/>
        <meta name="ocr-capabilities" content="ocr_page ocrx_word"/>
      </head>

      <body>
        <div class="ocr_page">
          <xsl:apply-templates/>
        </div>
      </body>
    </html>
  </xsl:template>

  <!-- Deal with the bounding-box. XXX: Can an attribute be returned from a template? -->
  <xsl:template match="ia:r">
    <xsl:attribute name="title">
      <xsl:text>bbox</xsl:text>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@l"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@t"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@r"/>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@b"/>
    </xsl:attribute>
  </xsl:template>

  <!-- Only use the first indicated token. -->
  <xsl:template match="ia:o[position()=1]">
    <xsl:value-of select="text()"/>
  </xsl:template>

  <!-- Each "node" corresponds to a word. -->
  <xsl:template match="ia:n">
    <span class="ocrx_word">
      <xsl:attribute name="id">
        <xsl:value-of select="generate-id()"/>
      </xsl:attribute>
      <xsl:apply-templates/>
    </span>
  </xsl:template>

  <!-- Recurse by default. -->
  <xsl:template match='*'>
    <xsl:apply-templates/>
  </xsl:template>

  <!-- Delete text which isn't handled explicitly. -->
  <xsl:template match="text()"/>
</xsl:stylesheet>
