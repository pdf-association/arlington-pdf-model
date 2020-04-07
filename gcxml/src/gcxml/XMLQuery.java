/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package gcxml;


import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
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
    
    // show keys that match specified pdf version
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
    
    // show all keys and their 'since version' attribute
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
    // show deprecated keys that match specified pdf version
    public void DeprecatedIn(String pdfVersion){
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try{
                String inputFile = inputFolder + file.getName();
                Document doc = domBuilder.parse(inputFile);
                doc.getDocumentElement().normalize();
                
                XPath xPath =  XPathFactory.newInstance().newXPath();
                
                String expression = "/OBJECT/ENTRY";	        
                NodeList nodeList = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);
                System.out.println("Keys deprecated in PDF version " + pdfVersion +" for object: " + file.getName().substring(0, file.getName().length()-4));
                int keyCount = 0;
                for (int i = 0; i < nodeList.getLength(); i++) {
                    Node nNode = nodeList.item(i);
                    //System.out.println("\nCurrent Element :" + nNode.getNodeName());
                    if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                        Element eElement = (Element) nNode;
                        String nodeName = eElement.getElementsByTagName("NAME").item(0).getTextContent();
                        String nodeDeprecatedIn = eElement.getElementsByTagName("DEPRECATEDIN").item(0).getTextContent();
                        if(pdfVersion.equals(nodeDeprecatedIn)){
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
    
    // show all keys and their 'deprecated in' attribute
    public void DeprecatedIn(){
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try{
                String inputFile = inputFolder + file.getName();
                Document doc = domBuilder.parse(inputFile);
                doc.getDocumentElement().normalize();
                
                XPath xPath =  XPathFactory.newInstance().newXPath();
                
                String expression = "/OBJECT/ENTRY";	        
                NodeList nodeList = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

                System.out.println("Deprecated keys for object: " + file.getName().substring(0, file.getName().length()-4));
                for (int i = 0; i < nodeList.getLength(); i++) {
                    Node nNode = nodeList.item(i);
                    //System.out.println("\nCurrent Element :" + nNode.getNodeName());
                    if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                        Element eElement = (Element) nNode;
                        String nodeName = eElement.getElementsByTagName("NAME").item(0).getTextContent();
                        String nodeDeprecatedIn = eElement.getElementsByTagName("DEPRECATEDIN").item(0).getTextContent();
                        if(!nodeDeprecatedIn.isEmpty()){
                            System.out.println("Key: " + nodeName + " - was deprecated in " + nodeDeprecatedIn);
                        }
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
    
    public void SchizophrenicObjects(){
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try{
                String inputFile = inputFolder + file.getName();
                Document doc = domBuilder.parse(inputFile);
                doc.getDocumentElement().normalize();
                
                XPath xPath =  XPathFactory.newInstance().newXPath();
                
                String expression = "/OBJECT/ENTRY";	        
                NodeList nodeList = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

                System.out.println("Object: " + file.getName().substring(0, file.getName().length()-4));

                for (int i = 0; i < nodeList.getLength(); i++) {
                    Node nNode = nodeList.item(i);
                    if (nNode.getNodeType() == Node.ELEMENT_NODE) {
 
                        Element eElement = (Element) nNode;
                        String nodeName = eElement.getElementsByTagName("NAME").item(0).getTextContent();
                        String nodeRequired = eElement.getElementsByTagName("REQUIRED").item(0).getTextContent();
                        
                        if((nodeName.equals("Type"))){
                            expression = "/OBJECT/ENTRY[NAME='Type']/VALUES/VALUE/TYPE/text()";
                            ArrayList<String> arrListTypes = evaluateXPath(doc, expression);

                            if(!arrListTypes.isEmpty()){  
                                Iterator<String> listIterator = arrListTypes.iterator();
                                while(listIterator.hasNext()){
                                     String nodeValue = listIterator.next();
                                    if(nodeValue.equals("name") || (nodeValue.equals("integer"))){
                                        if(nodeRequired.equals("false")){
                                            System.out.println(" contains key Type [" +nodeValue+ "] - optional");
                                        }else{
                                            System.out.println(" contains key Type [" +nodeValue+ "] - required");
                                        }
                                    }
                                }
                            }
                        }
                        if(nodeName.equals("Subtype")){
                            expression = "/OBJECT/ENTRY[NAME='Subtype']/VALUES/VALUE/TYPE/text()";
                            ArrayList<String> arrListTypes = evaluateXPath(doc, expression);

                            if(!arrListTypes.isEmpty()){
                                Iterator<String> listIterator = arrListTypes.iterator();
                                while(listIterator.hasNext()){
                                    String nodeValue = listIterator.next();
                                    if(nodeValue.equals("name") || (nodeValue.equals("integer"))){
                                        if(nodeRequired.equals("false")){
                                            System.out.println(" contains key Subtype [" +nodeValue+ "]- optional");
                                        }else{
                                            System.out.println(" contains key Subtype [" +nodeValue+ "] -required");
                                        }
                                    }
                                }
                            }                          
                        }
                        if(nodeName.equals("S")){
                            expression = "/OBJECT/ENTRY[NAME='S']/VALUES/VALUE/TYPE/text()";
                            ArrayList<String> arrListTypes = evaluateXPath(doc, expression);

                            if(!arrListTypes.isEmpty()){
                                Iterator<String> listIterator = arrListTypes.iterator();
                                while(listIterator.hasNext()){
                                    String nodeValue = listIterator.next();
                                    if(nodeValue.equals("name") || (nodeValue.equals("integer"))){
                                        if(nodeRequired.equals("false")){
                                            System.out.println(" contains key Subtype [" +nodeValue+ "] - optional");
                                        }else{
                                            System.out.println(" contains key Subtype [" +nodeValue+ "] - required");
                                        }
                                    }
                                }
                            }                          
                        }
                    } 
                }
                System.out.println();

                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (Exception ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }    
    }
    
    public void KeyOccurrenceCount(){
        
        HashMap<String, Integer> keyMap = new HashMap<String, Integer>();
        ArrayList<String> keys = new ArrayList<>();
        keys = getAllKeys();
        
        if(!keys.isEmpty()){
            Iterator<String> listIterator =  keys.iterator();
            while(listIterator.hasNext()){
                String key = listIterator.next();
                if(keyMap.containsKey(key)){
                    for (Entry<String, Integer> entry : keyMap.entrySet()) {
                         if (entry.getKey().equals(key)) {
                            int count = entry.getValue();
                            ++count;
                            keyMap.replace(key, count);
                    }
                }
                }else{
                    keyMap.put(key, 1);
                }
            }
        }
        
        printMap(keyMap);
    }
    
    private void printMap(Map mp) {
        Iterator it = mp.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry pair = (Map.Entry)it.next();
            System.out.println(pair.getKey() + " = " + pair.getValue());
        }
    }
    
    private static ArrayList<String> evaluateXPath(Document document, String xpathExpression) throws Exception 
    {
        XPathFactory xpathFactory = XPathFactory.newInstance();
         
        XPath xpath = xpathFactory.newXPath();
 
        ArrayList<String> values = new ArrayList<>();
        try
        {
            XPathExpression expr = xpath.compile(xpathExpression);
             
            NodeList nodes = (NodeList) expr.evaluate(document, XPathConstants.NODESET);
             
            for (int i = 0; i < nodes.getLength(); i++) {
                values.add(nodes.item(i).getNodeValue());
            }
                 
        } catch (XPathExpressionException e) {
            e.printStackTrace();
        }
        return values;
    }

    private ArrayList<String> getAllKeys() {
        
        ArrayList<String> allKeys = new ArrayList<>();
        
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try{
                String inputFile = inputFolder + file.getName();
                Document doc = domBuilder.parse(inputFile);
                doc.getDocumentElement().normalize();
                
                XPath xPath =  XPathFactory.newInstance().newXPath();
                
                String expression = "/OBJECT/ENTRY/NAME/text()";
                ArrayList<String> arrListTypes = evaluateXPath(doc, expression);
                
                
                 if(!arrListTypes.isEmpty()){
                    Iterator<String> listIterator = arrListTypes.iterator();
                        while(listIterator.hasNext()){
                        String keyName = listIterator.next();
                        //System.out.println(keyName);
                        allKeys.add(keyName);
                    }
                }

                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (Exception ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
        return allKeys;
    }
}
