/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package gcxml;


import java.io.File;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

/**
 *
 * @author fero
 */
public class XMLQuery {
    
    private String inputFolder;
    
    private DocumentBuilderFactory domFactory = null;
    private DocumentBuilder domBuilder = null;
    private Document newDoc = null;
    
    private File folder = null;
    private File[] files = null;
        
    public XMLQuery() {
        this.inputFolder = System.getProperty("user.dir") + "/xml/objects/";
        
        try {
          domFactory = DocumentBuilderFactory.newInstance();
          domBuilder = domFactory.newDocumentBuilder();
          newDoc = domBuilder.newDocument();
        } catch (FactoryConfigurationError exp) {
          System.err.println(exp.toString());
        } catch (ParserConfigurationException exp) {
          System.err.println(exp.toString());
        } catch (Exception exp) {
          System.err.println(exp.toString());
        }
        this.folder = new File(inputFolder);
        this.files = folder.listFiles();
    }
    
    public void SinceVersion(String pdfVersion){
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try{
                String inputFile = inputFolder + file.getName();
                Document doc = domBuilder.parse(inputFile);
                doc.getDocumentElement().normalize();
                
                XPath xPath =  XPathFactory.newInstance().newXPath();
                
                String expression = "/OBJECT/ENTRY";	        
                NodeList nodeList = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);
                System.out.println("Keys introduced in PDF version " + pdfVersion +" for object: " + file.getName().substring(0, file.getName().length()-4));
                int keyCount = 0;
                for (int i = 0; i < nodeList.getLength(); i++) {
                    Node nNode = nodeList.item(i);
                    //System.out.println("\nCurrent Element :" + nNode.getNodeName());
                    if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                        Element eElement = (Element) nNode;
                        String nodeName = eElement.getElementsByTagName("NAME").item(0).getTextContent();
                        String nodeSinceVersion = eElement.getElementsByTagName("SINCEVERSION").item(0).getTextContent();
                        if(pdfVersion.equals(nodeSinceVersion)){
                            System.out.println(nodeName);
                            keyCount++;
                        }
                    } 
                }
                if(keyCount == 0) {
                    System.out.println("No keys were found in this object.");
                }
                System.out.println();

                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }    
    }
    
        public void SinceVersion(){
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try{
                String inputFile = inputFolder + file.getName();
                Document doc = domBuilder.parse(inputFile);
                doc.getDocumentElement().normalize();
                
                XPath xPath =  XPathFactory.newInstance().newXPath();
                
                String expression = "/OBJECT/ENTRY";	        
                NodeList nodeList = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

                System.out.println("Keys for object: " + file.getName().substring(0, file.getName().length()-4));
                int keyCount = 0;
                for (int i = 0; i < nodeList.getLength(); i++) {
                    Node nNode = nodeList.item(i);
                    //System.out.println("\nCurrent Element :" + nNode.getNodeName());
                    if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                        Element eElement = (Element) nNode;
                        String nodeName = eElement.getElementsByTagName("NAME").item(0).getTextContent();
                        String nodeSinceVersion = eElement.getElementsByTagName("SINCEVERSION").item(0).getTextContent();
                        System.out.println("Key: " + nodeName + " - since version " + nodeSinceVersion);
                    } 
                }
                System.out.println();

                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }    
    }
    
}
